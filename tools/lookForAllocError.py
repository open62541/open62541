import sys
import subprocess
import os.path
import re
import os


project_src = sys.argv[1]
allocation_test_file = sys.argv[2]

valgrind_log = "check_allocation.log"
valgrind_flags = "--quiet --trace-children=yes --leak-check=full --run-libc-freeres=no --sim-hints=no-nptl-pthread-stackcache --track-fds=yes"
valgrind_command = "valgrind --error-exitcode=1 --suppressions=" + project_src + "/tools/valgrind_suppressions.supp " + valgrind_flags + " --log-file=" + valgrind_log +  " " +  allocation_test_file
check_allocation_command = "python " + project_src + "/tools/valgrind_check_error.py " + valgrind_log + " " + valgrind_command 

# Execute a command and output its stdout text
def execute(command):

    print("\n----------------Executing Tests ---------------------\n")

    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    # Poll process for new output until finished
    while True:
        nextline = process.stdout.readline().decode('utf-8')
        if nextline == '' and process.poll() is not None:
            break
        sys.stdout.write(nextline)
        sys.stdout.flush()

    return process.returncode


def write_lines_in_file(file, lines):
    for line in lines:
        file.write(line)

print("opening allocation failure files")

failurePoints = open("allocationFailurePoints", "r")
lines = failurePoints.readlines()
failurePoints.close()

print("making a copy of it")
failurePointsOriginal = open("allocationFailurePointsORIGINAL", "w+")
write_lines_in_file(failurePointsOriginal, lines)
failurePointsOriginal.close()

length = len(lines)

while True:
    
    part1 = lines[int(length/2):]
    part2 = lines[:int(length/2)]
    
    with open("allocationFailurePoints", "w") as failurePoints:
        write_lines_in_file(failurePoints, part1)
    
    print("##################### length is " + str(len(part1)))    
    ret_code = execute(check_allocation_command)
    
    if ret_code == 0: #no errors in this part
        with open("allocationFailurePoints", "w") as failurePoints:
            write_lines_in_file(failurePoints, part2)
        
        print("##################### length is " + str(len(part2)))
        ret_code = execute(check_allocation_command)
        if ret_code == 0: 
            print("2- The file doesn't have any line that make the test fail")
            exit(1)
        else:
            if len(part2) == 1:
                print("2- The line that breaks the tests is " + part2[0])
                exit(0)
            lines = part2
            
    else:
        if len(part1) == 1:
            print("1- The line that breaks the tests is " + part1[0])
            exit(0)
        lines = part1  
        
    length = len(lines)
        

    




