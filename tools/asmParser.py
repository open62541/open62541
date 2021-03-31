import subprocess
import io
import argparse

all_functions = []
all_address_to_break = []

# Represents a function in the code. 
# It has a name, and address (taken from the assembly code) and two lists: 
# one of the functions it calls, and another of the functions that call it
class Function():

    def __init__(self, name, address):
        self.name = name
        self.address = address
        self.i_call = []
        self.they_call_me = []

    def add_i_call(self, function):
        self.i_call.append(function)

    def add_they_call_me(self, function, address, line):
        self.they_call_me.append(TheyCallMe(function, address, line))

    def  __str__(self):
        return "Function " + self.name + " with address " + str(self.address)

# Represents a point in code that is calling another function.
# It contains the function object that is calling, and the address and the source code line (taken from the assembly code) where it is calling from 
class TheyCallMe():

    def __init__(self, function, address, line):
        self.function = function
        self.address = address
        self.line = line


# Get a function by name, and if not present, create it
def get_function_create(name, address):
    function = get_function_by_name(name) 
    if function is None:
        function = Function(name, int(address, 16))
        all_functions.append(function)

    return function
        
# Get the next assembly code address
def get_next_adress(lines):
    for i in range(len(lines)):
        if lines[i][0] == " ": #skip source code lines, since they start right at the beginning of the line
            return int(lines[i].split(":")[0].strip(), 16)

# Get the source code line, which was seen before
def get_call_line(lines):
    for i in range(len(lines) - 1, 0, -1):
        if lines[i][0] != " ":
            return lines[i][:-1] #remove the new line

# Given a list of lines representing the assembly code, it parses it and store the functions,
# where they are calling and who's calling them
# This code should be changed for another assembly code type
def parse_asm(lines):
    print("File has " + str(len(lines)) + " lines")
    
    for i in range(len(lines)):
        line = lines[i]
        
        # Skip empty lines
        if "" == line:
            continue
        
        # The first line of a function contains ">:"
        if ">:" in line:
            function_pointer_to_call = None # clear the stored function pointer 
            function_address = line.strip().split(' ')[0]
            function_name = line.strip().split(' ')[1][1:-2]
            current_function = Function(function_name, int(function_address, 16))
            all_functions.append(current_function)
            
        # The function is calling another function
        elif "callq" in line:
            function_called_info = line.split("callq")[1].strip().split(" ")
            function_called = None
            
            # A function pointer is being called
            if len(function_called_info) != 2: 
                if function_pointer_to_call is None:
                    continue
                else:
                    function_called = function_pointer_to_call
            
            else: # A normal function is being called
                called_function_address = function_called_info[0]
                called_function_name = function_called_info[1][1:-1]
                
                function_called = get_function_create(called_function_name, called_function_address) 
      
            current_function.add_i_call(function_called)
            current_address = get_next_adress(lines[i+1:]) # For some reason, the next address is the calling point in the function
            current_line = get_call_line(lines[:i - 1])
            function_called.add_they_call_me(current_function, current_address, current_line)
        
        # Where a function pointer is assigned to the function to call, this "lea" command is seen
        elif "lea " in line:
            function_called_info = line.split("#")
            if len(function_called_info) != 2:
                #print("Line" + str(i + 1) + ":\n" + line + "\nis loading a function pointer but not couldnt'find the information about it")
                continue
            function_called_info = function_called_info[1].strip().split(" ")
            called_function_address = function_called_info[0]
            called_function_name = function_called_info[1][1:-1]
            function_pointer_to_call = get_function_create(called_function_name, called_function_address) 

# Get a function object by its name
def get_function_by_name(name):
    for function in all_functions:
        if function.name == name:
            return function

# Given a function name and a stack, go backwards to find all points where the initial
# function name is called.
# The stack's purpose is to avoid recursive function calls, which will get this function stuck
def create_traceback(function_name, stack):
    function = get_function_by_name(function_name)
    if None == function:
        return

    for they_call_me_function in function.they_call_me:
        breaking_place_to_add = (they_call_me_function.address, they_call_me_function.line)
        if breaking_place_to_add not in all_address_to_break:
            all_address_to_break.append(breaking_place_to_add)

        # If the calling function is already in the stack, we don't go further since it will get stuck
        if they_call_me_function not in stack:
            
            stack.append(they_call_me_function)
            create_traceback(they_call_me_function.function.name, stack)
            # After the recursion finishes, we pop the recently added function from stack.
            stack.pop()


current_function = None
function_pointer_to_call = None

# construct the argument parser and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-b", "--binary", required = True, help = "Path to binary file")
ap.add_argument("-f", "--function", required = True, help = "Functions to look for", action = "append")
ap.add_argument("-e", "--exclude", help = "Exclude regexp pattern from source file names", action = "append")

args = vars(ap.parse_args())

binary = args["binary"]
function_names = args["function"]
exclusions = args.get("exclude")
if exclusions is None:
    exclusions = [] #avoids problems later when using the variable


print("Parse assembly file from " + binary)
print("Exclude source files with patterns: " + str(exclusions))
print("Create call tree for functions: " + str(function_names))

assemblyText = io.StringIO(subprocess.check_output("objdump -dl -j .text " + binary +  " | tail -n +7", shell=True).decode('utf-8'))
lines = assemblyText.readlines()
parse_asm(lines)

print("File already parsed. We'll build now all function-chains that might call the following functions: " + str(function_names))

for function_name in function_names:
    function = get_function_by_name(function_name)
    stack = [TheyCallMe(function, 0, "")]
    create_traceback(function.name, stack)


no_of_breaks = 0

with open("allocationFailurePoints", "w") as memoryFile:
    for address in all_address_to_break:
        include = True
        for exclusion in exclusions:
            if exclusion in address[1]:
                include = False
                break
        
        if not include:
            continue
        
        lineToWrite = str(address[0]) + "," + str(address[1]) + "\n"
        memoryFile.write(lineToWrite)
        no_of_breaks += 1
        
    print(str(no_of_breaks) + " possible failure points were found")


