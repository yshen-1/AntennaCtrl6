# Antenna Control
# Lou Chomas, Yuyi Shen
# Carnegie Mellon University
#
# May 9, 2018
#
# Sends commands to the antenna control board over USB to turn switches on and off.
#


import serial
import sys
import math
import threading
import queue
import argparse

BAUDRATE = 115200
# QUEUE_LOCK allows synchronizing queue access explicitly
QUEUE_LOCK = threading.Lock()
# THREAD_LOCK indicates thread termination
THREAD_LOCK = threading.Lock()

# will hold all input read, until the work thread has chance
# to deal with it
input_queue = queue.Queue()

#Configures serial port
def configure_serial(serial_port):
    return serial.Serial(
        port=serial_port,
        baudrate=BAUDRATE,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_TWO,
        bytesize=serial.EIGHTBITS,
        writeTimeout = 0.1,
        timeout = 0.4,
        rtscts=False,
        dsrdtr=False,
        xonxoff=False
    )


def send_cmd(com_port):
    print("Opening port " + com_port)
    try:
        ser = configure_serial(com_port)
        if not ser.isOpen():
            raise Exception
    except:
        print( "Opening serial port {} failed.".format(com_port) )
        raise
        exit()
    
    while (1):
        try:
            # Block until queue is unlocked.
            QUEUE_LOCK.acquire()
            if (input_queue.empty()):
                QUEUE_LOCK.release()
                continue
            next_cmd = input_queue.get(timeout=0.1)
            QUEUE_LOCK.release()
            print("Sending cmd:" + next_cmd, end="")
            if (next_cmd == "\n"):
                print("Terminating work thread.")
                break
            ser.write(next_cmd.encode('utf-8'))
        except:
            continue
        
        try: 
            return_msg = ser.readline()
        except:
            continue
        print(return_msg)
        print()
        
    ser.close()
    THREAD_LOCK.release()
    return

def input_cleanup():
    print()
    while not input_queue.empty():
        line = input_queue.get()
        print("Didn't get to work on this line:", line, end='')

'''
Initializes argument parser
'''
def init_parser():
    program_descrip = ("This python program allows the user " + 
                      "to send arbitrary bytes \nthrough an open" + 
                      " serial port to an appropriately configured \n" +
                      "USB device, such as the 6 element board " + 
                      "or 70 element board. \nEach series of bytes " + 
                      "sent through the port comprises a board \n" + 
                      "command, and is terminated by a newline.\n\n" +
                      "A valid board command has the following format:\n" +
                      "<p><EPMnumber><newline> \n" + 
                      "p is a single character that is either + or -\n" +
                      "and signifies the polarity of the EPM pulse desired.\n" +
                      "For example, the command '+70' commands the board\n" +
                      "to send a positive pulse to EPM 70.\n\n" +
                      "Commands can be manually inputted via cmdline or be fed\n" + 
                      "in using an appropriately formatted text file. To\n" +
                      "use a text file to send board commands, specify the filename\n" +
                      "with the -f option.\n\n" +
                      "See the sample cmds.txt file for an example of proper\n" +
                      "formatting.\n")
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter,
        description=program_descrip)
    port_help = "Serial port for communicating with board"
    parser.add_argument('-port', required=True, help=port_help)
    file_help = "Optional filename of text file containing pre-generated commands"
    parser.add_argument('-f', help=file_help)
    return parser

'''
Directly read commands from stdin if no file is specified.
'''
def read_from_stdin():
    try:
        for line in sys.stdin:
            if (THREAD_LOCK.acquire(blocking = False)):
                input_cleanup()
                break
            if line:
                QUEUE_LOCK.acquire()
                input_queue.put(line)
                QUEUE_LOCK.release()
    except KeyboardInterrupt:
        QUEUE_LOCK.acquire()
        input_queue.put("\n")
        QUEUE_LOCK.release()


'''
If a file is specified, push each line
in the file onto the queue. Each line
of the file comprises a single command.
'''
def read_from_file(file_target):
    cmd_file = open(file_target, 'r')
    newline = cmd_file.readline()
    while (newline != ""):        
        if (THREAD_LOCK.acquire(blocking = False)):
            input_cleanup()
            break
        if ("\n" not in newline):
            newline = newline + "\n"
        QUEUE_LOCK.acquire()
        input_queue.put(newline)
        QUEUE_LOCK.release()
        newline = cmd_file.readline()
    QUEUE_LOCK.acquire()
    input_queue.put("\n")
    QUEUE_LOCK.release()
    cmd_file.close()
    return

def main():
    parser = init_parser()
    args = parser.parse_args(sys.argv[1:])
    THREAD_LOCK.acquire()
    usb_th = threading.Thread(target=send_cmd, args=(args.port,))
    usb_th.start()
    if (args.f == None):
        read_from_stdin()
    else:
        read_from_file(args.f)

main()