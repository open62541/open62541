import sys
import subprocess


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

testedFailurePoints = open("testedAllocationFailurePoints", "r")
lines = testedFailurePoints.readlines()
testedFailurePoints.close()


failure_lines = []

for line in lines:
    
    with open("allocationFailurePoints", "w") as failurePoints:
        write_lines_in_file(failurePoints, line)
    
    
    if execute(check_allocation_command) != 0:
        failure_lines.append(line)
        

if len(failure_lines) !=0:
    print("---------------------------------------------  THE FOLLOWING LINES FAIL ----------------------------------")
    
    for line in failure_lines:
        print("LINE:\n" + line + "\n")
        with open("allocationFailurePoints", "w") as failurePoints:
            write_lines_in_file(failurePoints, line)
        
        execute(check_allocation_command)
