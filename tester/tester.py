#!/usr/bin/env python3

"""
MTB-UNI v4 test script

Run this script to test all inputs & outputs of the module are wokring properly.
Use 3 MTB-UNI modules for the test. (1) Tested module (2) Inputs module
(3) Outputs module. Connect outputs of (2) to the inputs of (1). Connect outputs
of (2) to the inputs of (3). Boundary modules are used for reading outputs
of the tested module and settings inputs of the tested module.
Connect IO pins 1-1 on PCB, e.g. output F of module (2) to input 0 of module (1).

Usage:
  tester.py [options] <tested_addr> <inputs_addr> <outputs_addr>

Options:
  -s <servername>    Specify MTB Daemon server address [default: localhost]
  -p <port>          Specify MTB Daemon port [default: 3841]
  -v                 Verbose
  -h --help          Show this screen.
  -w --wait          Wait when tests fails
"""

import socket
from docopt import docopt  # type: ignore
import traceback
from typing import Any, Dict
import json
import sys
from time import sleep

PROPAGATE_DELAY: int = 0.05  # 50 ms


class EDaemonResponse(Exception):
    pass


def request_response(socket, verbose: bool,
                     request: Dict[str, Any]) -> Dict[str, Any]:
    to_send = request
    to_send['type'] = 'request'
    if verbose:
        print(to_send)
    socket.send((json.dumps(to_send)+'\n').encode('utf-8'))

    while True:
        data = socket.recv(0xFFFF).decode('utf-8').strip()
        messages = [json.loads(msg) for msg in data.split('\n')]
        for message in messages:
            if verbose:
                print(message)
            if message.get('command', '') == request['command']:
                if message.get('status', '') != 'ok':
                    raise EDaemonResponse(
                        message.get('error', {}).get('message', 'Uknown error!')
                    )
                return message


def uni_set_outputs(socket, verbose: bool, addr: int, value: int) -> None:
    request_response(socket, verbose, {
        'command': 'module_set_outputs',
        'address': addr,
        'outputs': {
            str(porti): {'type': 'plain', 'value': 1 if (value & (1 << porti)) > 0 else 0}
            for porti in range(0, 16)
        },
    })


def get_inputs(socket, verbose: bool, module: int) -> int:
    response = request_response(socket, verbose, {
        'command': 'module',
        'address': module,
        'state': True,
    })
    module_ = response['module']
    module_spec = module_[module_['type']]
    if 'state' not in module_spec:
        raise EDaemonResponse(
            'No state received - is module active? Is it in bootloader?'
        )
    return module_spec['state']['inputs']['packed']


def reverse_bits(n: int, no_of_bits: int):
    result = 0
    for i in range(no_of_bits):
        result <<= 1
        result |= n & 1
        n >>= 1
    return result


def io_to_str(n: int) -> str:
    result = ''
    for i in range(16):
        result += '1' if n & (1 << i) else '0'
        if i == 7:
            result += ' '
    return result


def assert_in_eq(nin: int, nout: int) -> None:
    assert nin == reverse_bits(nout, 16), \
        f'Inputs set to {io_to_str(reverse_bits(nout, 16))}, but are {io_to_str(nin)}'


def test_inputs_single_bit(socket, verbose: bool, addr_tested: int, addr_inputs: int,
                           addr_outputs: int) -> None:
    for i in range(16):
        uni_set_outputs(socket, verbose, addr_inputs, 1 << i)
        sleep(PROPAGATE_DELAY)
        inputs = get_inputs(socket, verbose, addr_tested)
        assert_in_eq(inputs, 1 << i)


def test_inputs(socket, verbose: bool, addr_tested: int, addr_inputs: int,
                addr_outputs: int) -> None:
    print('[....] Testing inputs...')
    test_inputs_single_bit(socket, verbose, addr_tested, addr_inputs, addr_outputs)
    print('[ OK ] Input tests passed')


def main() -> None:
    args = docopt(__doc__)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args['-s'], int(args['-p'])))

    try:
        test_inputs(sock, args['-v'], int(args['<tested_addr>']), int(args['<inputs_addr>']),
                    int(args['<outputs_addr>']))
        print('[INFO] All tests done')
    except AssertionError:
        print(traceback.format_exc(), end='')
        if args['--wait']:
            while True:
                sock.recv(0xFFFF).decode('utf-8').strip()
        sys.exit(1)
    except EDaemonResponse as e:
        sys.stderr.write(str(e)+'\n')
        sys.exit(1)


if __name__ == '__main__':
    main()
