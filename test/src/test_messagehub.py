import socket
import time
import sys
import subprocess
from subprocess import Popen
import argparse

from messagehub_test_tcp_connection import test_tcp_connection
from messagehub_test_valid_handshake import test_valid_handshake
from messagehub_test_invalid_handshake import test_invalid_handshake
from messagehub_test_parallel_connections import test_parallel_connections

default_registry_path = "../../../build/bin/rcregistry"
default_output_file = 'stdout.txt'
default_number_loops = 1


def start_rcregistry(path, output_file):
    if output_file != "stdout":
        output = open(output_file, 'w+')
        registry = Popen([path], stdout=output, stderr=output, universal_newlines=True)
    else:
        registry = Popen([path], universal_newlines=True)
    time.sleep(3)
    return registry


def loop_test(number_loops, test_function):
    success = True
    for i in range(number_loops):
        success = test_function()
        if success == False:
            print("TEST FAILED")
            break
    return success


def parse_arguments():
    parser = argparse.ArgumentParser(description='Test rcregistry.')

    parser.add_argument('--path',
                        type=str,
                        nargs='?',
                        default=default_registry_path,
                        help='the path to rcregistry')
    
    parser.add_argument('--loops', 
                        type=int,
                        nargs='?',
                        default=default_number_loops,
                        help='the number of loops')

    parser.add_argument('--output', 
                        type=str,
                        nargs='?',
                        default=default_output_file,
                        help='the output file for stdout and stderr')

    parser.add_argument('--test', 
                        type=int,
                        nargs='?',
                        default=0,
                        help='the tests to run (0=all)')

    return parser.parse_args()


def run_test():
    success = False
    args = parse_arguments()
    registry = start_rcregistry(args.path, args.output)

    if args.test == 1:
        success = loop_test(args.loops, test_tcp_connection)
        
    elif args.test == 2:
        success = loop_test(args.loops, test_valid_handshake)
        
    elif args.test == 3:
        success = loop_test(args.loops, test_invalid_handshake)

    elif args.test == 4:
        success = loop_test(args.loops, test_parallel_connections)
        
    else:
        success = (loop_test(args.loops, test_tcp_connection)
                   and loop_test(args.loops, test_valid_handshake)
                   and loop_test(args.loops, test_invalid_handshake)
                   and loop_test(args.loops, test_parallel_connections))
    

    registry.terminate() 
    return success
    

if __name__ == "__main__":
    success = False
    try:
        success = run_test()
    except Exception as ex:
        print(ex)
        
    if success:
        sys.exit(0)    
    else:
        sys.exit(1)
