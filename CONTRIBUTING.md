# Contributing to open62541

Contributions to open62541 include code, documentation, answering user
questions, running the project's infrastructure, and advocating for all types of
open62541 users.

The open62541 project welcomes all contributions from anyone willing to work in
good faith with other contributors and the community. No contribution is too
small and all contributions are valued.

This guide explains the process for contributing to the open62541 project's core
repository and describes what to expect at each step. Thank you for considering
these point.

Your friendly open62541 community!

## Code of Conduct

The open62541 project has a [Code of Conduct](./CODE_OF_CONDUCT.md) that *all*
contributors are expected to follow. This code describes the *minimum* behavior
expectations for all contributors.

## Pull Requests

Everybody can propose a pull request (PR). But only the core-maintainers of the
project can merge PR.

### Minimal requirements for a PR

The following are the minimal requirements that every PR needs to meet.

- **Pass Continuous Integration (CI)**: Every PR has to pass our CI. This
  includes compilation with a range of compilers and for a range of target
  architectures, passing the unit tests and no detected issues with static code
  analysis tools.

- **Code-Style**: Please consider the
  [Code-Style](#code-style)
  recommendations when formatting your code.

- **Signed CLA**: Every contributor must sign the Contributor License Agreement
  (CLA) before we can merge his first PR. The signing can be done online. The
  link automatically appears on the page of the first PR. In addition, the CLA
  text can be accessed [here](https://cla-assistant.io/open62541/open62541).

## Commit and PR Hygiene

We have very precise rules over how our git commit messages can be formatted.  This leads to **more
readable messages** that are easy to follow when looking through the **project history**.  But also,
we use the git commit messages to **generate the change log**.

This convention is identical to the [Conventional Commits](https://www.conventionalcommits.org) specification or the one used by Angular.

### Commit Message Format
Each commit message consists of a **header**, a **body** and a **footer**.  The header has a special
format that includes a **type**, a **scope** and a **subject**:

```text
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

The **header** is mandatory and the **scope** of the header is optional.

Any line of the commit message cannot be longer 100 characters! This allows the message to be easier
to read on GitHub as well as in various git tools.

The footer should contain a [closing reference to an issue](https://help.github.com/articles/closing-issues-via-commit-messages/) if any.

Samples: (even more [samples](https://github.com/angular/angular/commits/master))

```text
docs(server): add function documentation
```
```text
fix(core): fix parsing of endpoint url

Parsing of endpoint urls now also supports https
```

### Revert
If the commit reverts a previous commit, it should begin with `revert: `, followed by the header of the reverted commit. In the body it should say: `This reverts commit <hash>.`, where the hash is the SHA of the commit being reverted.

### Type
Must be one of the following:

- **build**: Changes that affect the build system or external dependencies
- **ci**: Changes to our CI configuration files and scripts (example scopes: travis, appveyor, fuzz)
- **docs**: Documentation only changes
- **feat**: A new feature
- **fix**: A bug fix
- **perf**: A code change that improves performance
- **refactor**: A code change that neither fixes a bug nor adds a feature
- **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
- **test**: Adding missing tests or correcting existing tests

### Scope
The scope is optional, but recommended to be used. It should be the name of the component which is affected (as perceived by the person reading the changelog generated from commit messages).

The following is the list of supported scopes:

- **arch**: Changes to specific architecture code in `root/arch`
- **client**: Changes only affecting client code
- **core**: Core functionality used by the client and server
- **ex**: Example code changes
- **mt**: Changes specifically for multithreading
- **nc**: Nodeset compiler
- **pack**: Packaging setting changes
- **plugin**: Change to any (optional) plugin
- **pubsub**: Changes to the pubsub code
- **sec**: Changes to security, encryption, etc.
- **server**: Changes only affecting server code

### Subject
The subject contains a succinct description of the change:

- use the imperative, present tense: "change" not "changed" nor "changes"
- don't capitalize the first letter
- no dot (.) at the end

### Body
Just as in the **subject**, use the imperative, present tense: "change" not "changed" nor "changes".
The body should include the motivation for the change and contrast this with previous behavior.

### Footer
The footer should contain any information about **Breaking Changes** and is also the place to
reference GitHub issues that this commit **Closes**.

**Breaking Changes** should start with the word `BREAKING CHANGE:` with a space or two newlines. The rest of the commit message is then used for this.

## General commit hygiene

We are using the [Conventional Commits](https://www.conventionalcommits.org) specification (see previous section).

These sites explain a core set of good practice rules for preparing a PR:

- https://wiki.openstack.org/wiki/GitCommitMessages
- https://nvie.com/posts/a-successful-git-branching-model/

The following points will be especially looked at during the review.

- **Separation of Concerns**: Small changes are much easier to review.
  Typically, small PR are merged much faster. For larger contributions, it might
  make sense to break them up into a series of PR. For example, a PR with a new
  feature should not contain other commits with only stylistic improvements to
  another portion of the code.

- **Feature Commits**: The same holds true for the individual PR as well. Every
  commit inside the PR should do one thing only. If many changes have been
  applied at the same time, `git add --patch` can be used to partially stage and
  commit changes that belong together.

- **Commit Messages**: Good commit messages help in understanding changes.
  See previous section.

- **Linear Commit History**: Our goal is to maintain a linear commit history
  where possible. Use the `git rebase` functionality before pushing a PR. Use
  `git rebase --interactive` to squash bugfix commits.

### Review Process

These labels can be used for the PR title to indicate its status.

- `[WIP]`: The PR is work in progress and at this point simply informative.
- `[Review]`: The PR is ready from the developers perspective. He requests a review from a core-maintainer.
- `[Discussion]`: The PR is a contribution to ongoing technical discussions. The PR may be incomplete and is not intended to be merged before the discussion has concluded.

The core-maintainers are busy people. If they take especially long to react,
feel free to trigger them by additional comments in the PR thread. Again, small
PR are much faster to review.

It is the job of the developer that posts the PR to rebase the PR on the target
branch when the two diverge.

### Changes to the public API

The *public* API is the collection of header files in the /include folder.

Changes to the public API are under especially high scrutiny. Public API changes
are best discussed with the core-maintainers early on. Simply to avoid duplicate
work when changes to the proposed API become necessary.

You can create a special issue or PR just for the sake of discussing a proposed
API change. The actual implementation can follow later on.

## Code Style

###  General
1. Avoid the use of [Magic Numbers](https://en.wikipedia.org/wiki/Magic_number_(programming)#Unnamed_numerical_constants).
   For example use `UA_STATUSCODE_BADINTERNALERROR` instead of `0x80020000`
2. Avoid using global variables. If you for some reason really need to use global variables, prefix them with `g_` so that it is immediately clear in the code, that we are dealing with a global variable.
3. Almost all of the code is indented by four (4) spaces and regardless of which is better, spaces or tabs, you should indent with four spaces in order to be consistent.
4. If you find yourself copy-pasting code, consider refactoring it into a function and calling that.
   Try keeping functions short (about 20-30 lines, that way they will in most cases fit onto the screen).
   In case of simple switch statements for example this can be exceeded, but there is not much complexity that needs to be understood.
   Most of the time, if functions are longer than 30 lines, they can be refactored into smaller functions that make their intent clear.
5. Use of comments

Use C-style comments as follows:

```c
/* Lorem ipsum dolor sit amet */

/* Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy
 * eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam
 * voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita
 * kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem
 * ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod
 * tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.
 * At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd
 * gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. */
```

Note that public header files (in the /include directory) use comments beginning with a double-star to generate documentation. Just follow along the formatting that is already in place.

```c
/** This is documentation */
```

Every (good) editor can reflow comments for nice formatting.

Do not use C++ comments (double-slash `//`).

See the following general rules for comments: [\[1\]](https://www.kernel.org/doc/Documentation/CodingStyle)
> Comments are good, but there is also a danger of over-commenting.  NEVER
> try to explain HOW your code works in a comment: it's much better to
> write the code so that the _working_ is obvious, and it's a waste of
> time to explain badly written code.

> Generally, you want your comments to tell WHAT your code does, not HOW.
> Also, try to avoid putting comments inside a function body: if the
> function is so complex that you need to separately comment parts of it,
> you should probably go back to chapter 6 for a while.  You can make
> small comments to note or warn about something particularly clever (or
> ugly), but try to avoid excess.  Instead, put the comments at the head
> of the function, telling people what it does, and possibly WHY it does
> it.

### 1 Packages:
1. "Package" = organizational element to aggregate methods, types, constants.
2. Package are underline-aggregated or CamelCased, lowercase and in singular formal

`examples: services_view, encoder, linkedList`
3. Pseudo-Packages can be used for consolidation reasons (e.g. enclose dozens built-in types)
   Pseudo-Packages' name should be using plural to focus on the consolidation aspect

`examples: basictypes to aggregate all the hand coded basic types`

### 2 Source file organization:
1. every file starts with a prefix ua_ followed by the package name
   examples: `ua_encoder.c, ua_transport_binary.c, ua_basictypes.c`
2. source files are located in the /src folder
3. source files can be further aggreagated on the file-system level, i.e.
```
	/src/.
		ua_encoder.c
		...	
		util/.
			ua_indexedList.c
			...
```
	NOTE: file-system organization *is not* reflected in the source code

### 3 Header files
1. The header files for the public API are stored in the /include folder and have the same name as .c files
2. The /include folder is a flattened /src folder e.g. it does not contain subdirectories

### 4 Methods and instances
1. Methods and instances are prefixed with UA_packageName
2. Methods and instances are camelCased starting with *lower* case
   Examples: `UA_encoder_encode`, `UA_Int32_calculateSize`
3. Use const on variables that will not be changed.
   For example in functions, make all parameters const, that are not used as out arguments.
   When declaring local variables in a function make them const, if you know that they will not be modified.
   When using pointers, remember to declare both the pointer and the actual type as const, if they are never modified:
   `const int *const myInt`
4. In- variables should be always *in front* of the out- variables in argument lists

### 5 Types
1. Types are prefixed with `UA_packageName`.
2. Types are CamelCased starting with *upper* case. Example: `UA_ResponseHeader`

3. Builtin datatypes do not have a package name. Example: `UA_Int32, UA_Int64`

### 6 Constants, enums, defines
1. Constants and defines are in full capital letters, eg.: `MY_CONST`
2. Constants should be defined in a matching package's .h file, if they are part of the interface.
   Otherwise they should be kept in the .c file in order to not pollute the interface.
3. Enums are preferred when defining several related constants, for example the different states of a secure channel:
```
typedef enum {
    UA_SECURECHANNELSTATE_FRESH,
    UA_SECURECHANNELSTATE_OPEN,
    UA_SECURECHANNELSTATE_CLOSED
} UA_SecureChannelState;
```

### 7 Variable naming
1. Variables should be descriptive and use camelCase starting with a lower case letter.
2. Some commonly known abbreviations or variable name conventions can be used.
   This includes for example loop variables like `i` and temporary variables like `tmp`.
   Use common sense, and think about if you would still understand your own code if you looked at it two weeks in the future.
3. Do not use Hungarian notation. The code base so far does not use it and neither should you, to be consistent.
   For some reasons, why Hungarian notation might be bad, see [here](https://stackoverflow.com/questions/111933/why-shouldnt-i-use-hungarian-notation)

### 8 Formatting
1. Braces: Opening braces start on the same line as a function, if statement, etc. and the closing brace is placed on a separate line.
   If the next keyword after a closing braces logically belongs to the previous keyword (i.e. if and else), the keyword is placed next to the closing brace on the same line with one space in between. Example:
    ```c
    if(a) {
        doSomething();
    } else if(b) {
        doSomething();
    } else doSomething();
    ```
2. Switch statement: Do not indent the case keyword and place the content of each case in new lines that are indented.
   If using a code block to limit the scope of variables, follow the above convention. Example:
    ```c
    switch(a) {
    case 0:
        doA();
    case 1: {
        doB();
        doC();
    }
    }
    ```
3. Align multiline expressions. Example:
    ```c
    int a = 1 +
            2 +
            3;
    
    int b = ((1) + 2) -
            (4 * 5 / 6 % 7);
    ```
4. Keep the number of blank lines to a minimum, but leave some if it makes the code more readable.
5. Surround binary operators with spaces except for member access operators. Do not use spaces around unary operators.
6. Do not place a space between a keyword and following parentheses.
7. Do not place spaces within parentheses.
8. Place a space before, but not after a pointer symbol (\*). Example: `const int *const **const myPtr;`

If you are using CLion you can configure your IDE by importing [this scheme](https://gist.github.com/Infinity95/c5c743180b447863aff9e5c76dfde0bb).

### 9 Tests
1. tests are contained in `/tests` dir and start with check_ prefix followed by a package name (corresponding .c file)

   `example: check_encoder.c, check_securityLayer.c`

2. `check_main.c` file is used to aggregate test cases that are executed

### 10 Checking returns, error logging and cleanup

Error handling and corresponding reactions like cleanup, logging or updating
internal error states is important for a stable program flow.

This section firstly describes specific macros that are used for checking return 
values in various situations in [Check Macros](#check-macros).
Secondly, [Cleanup](#cleanup) describes in which situations cleanup routines should be
used and how to use them in combination with the check macros.

#### Check Macros

For checking return values within functions there are specific macros that are used.
Handling check values and errors belongs to three different categories:

1. Handling `UA_StatusCode` check values
2. Handling memory allocation errors  
3. Handling general `boolean` check values

A very frequent use case is the checking of a `UA_StatusCode` return value and performing
a specific action upon error. The workflow and usage is the same for all of the above categories.

The basic check macro follows the structure `UA_CHECK_STATUS(STATUSCODE, EVAL_ON_ERROR)`, performing
a check on the `STATUSCODE` and execute `EVAL_ON_ERROR` if an erroneous state is detected.

The below example thus checks `rv` and executes `return rv` on error.

```c
UA_StatusCode rv = someAction();
UA_CHECK_STATUS(rv, return rv);
```

Similar to the above there exist logging variants for each check macro that logs to a specific
logging level. The macro names are appended with a suffix `_FATAL`, `_ERROR`, `_WARN` or `_INFO`.

In this code snippet, the status code gets checked with a specified error action, while logging a specified error level message:

```c
UA_Logger *logger = &server->config.logger;

UA_StatusCode rv = someAction();
/* logging "error" level with the "_ERROR" suffix */
UA_CHECK_STATUS_ERROR(rv, return rv, logger, UA_LOGCATEGORY_SERVER, "my message");
```

Make sure to check out more examples below for applications of different macros.

#### Cleanup

In some situations it is nice to use the `goto` command in C. This is the case
e.g. for jumping to cleanup routines that are used to perform a series of commands.

For the development of `open62541` we employ a specific rule for which cleanup routines 
should be used: **If the same cleanup routine is used from at least two places in the same function
then this routine should be called via a `goto` statement.**

In this example e.g. only the second error check needs to free up some memory upon encountering
an error, thus a jump to a cleanup routine is not indicated.

```c
void noCleanupRoutine(void *data) {
   
    data = malloc(sizeof(int));
    UA_CHECK_MEM(data, return UA_STATUSCODE_BADOUTOFMEMORY);
     
    UA_StatusCode rv = do_something();
    UA_CHECK_STATUS(rv, free(data); return rv);
    
    return UA_STATUSCODE_GOOD;
}
```

The below example on the other hand, shows two functions that use the same
routine for cleanup. A `goto` routine should thus be used.

```c
void yesCleanupRoutine(void *data) {
    
    data = malloc(sizeof(int));
    UA_CHECK_MEM(data, return UA_STATUSCODE_BADOUTOFMEMORY);
  
    /* jumps to cleanup routine upon encountering a bad statuscode */
    UA_StatusCode rv = do_something();
    UA_CHECK_STATUS(rv, goto cleanup);
 
    /* jumps to the same cleanup routine */
    rv = do_something_else();
    UA_CHECK_STATUS(rv, goto cleanup);
   
    return UA_STATUSCODE_GOOD;
    
cleanup:
    free(data);
    return rv;
}
```

##### More Examples

###### 2. Handling `UA_StatusCode` check values

In this example only the status codes are checked and the corresponding
action is evaluated on error. There is no message logging included.

```c
static UA_StatusCode
foo(int *errorCounter) {
    
    UA_StatusCode rv = do_something();
    /* if rv != UA_STATUSCODE_GOOD then "return rv" gets evaluated */
    UA_CHECK_STATUS(rv, return rv);
  
    rv = do_another_thing();
    
    /* 
    EVAL_ON_ERROR can take multiple statements
    (e.g. first modifying some value then return the error code) 
    */
    UA_CHECK_STATUS(rv, rv = UA_STATUSCODE_BAD; errorCounter++; return rv);

    return UA_STATUSCODE_GOOD;
}
```

In this example status codes are checked while logging message with different
levels and a specified `LOGCATEGORY` are generated with a given logger.

```c
static UA_StatusCode
foo(UA_Server *server) {
    
    /* assign the logger for later simple use */
    UA_Logger *logger = &server->config.logger;
    
    UA_StatusCode rv = do_something_error();

    /* 
    if rv != UA_STATUSCODE_GOOD then 
    an error logging message is generated and 
    "return rv" gets evaluated
    */
    UA_CHECK_STATUS_ERROR(rv, return rv,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My error logging message with special info: %d", 42);

    rv = do_something_warn();
    UA_CHECK_STATUS_WARN(rv, return rv,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My warning logging message with special info: %d", 42);

    rv = do_something_info();
    UA_CHECK_STATUS_INFO(rv, return rv,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My info logging message with special info: %d", 42);

    rv = do_something_fatal();
    UA_CHECK_STATUS_FATAL(rv, return rv,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My fatal logging message with special info: %d", 42);


    return UA_STATUSCODE_GOOD;
}
```

###### Short check examples for `boolean` and memory checking

All the other check macros function analogously to the above `UA_CHECK_STATUS` examples,
with a short example to explain the basic usage:

```c
static UA_StatusCode
foo() {
 
    /* assign the logger for later simple use */
    UA_Logger *logger = &server->config.logger;
    
    UA_Boolean mustBeTrue = do_something();
    /* if mustBeTrue != true then "return UA_STATUSCODE_BAD" gets evaluated */
    UA_CHECK(mustBeTrue, return UA_STATUSCODE_BAD);
   
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    /*
    EVAL_ON_ERROR can take multiple statements
    (e.g. first assigning some value then return the error code) 
    */
    UA_CHECK(mustBeTrue, rv = UA_STATUSCODE_BAD; return rv);

    UA_Boolean mustBeTrue = do_something_else();
    /* 
    if mustBeTrue != true then 
    an error logging message is generated and 
    "return UA_STATUSCODE_BAD" gets evaluated 
    */
    UA_CHECK_ERROR(mustBeTrue, return UA_STATUSCODE_BAD,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My logging message with special info: %d", 42);

 
    void *data = malloc(1, 1);
    UA_CHECK_MEM(data, return UA_STATUSCODE_BADOUTOFMEMORY);
   
    void *data2 = malloc(1, 1);
    UA_CHECK_MEM_ERROR(data2, return UA_STATUSCODE_BADOUTOFMEMORY,
                   logger, UA_LOGCATEGORY_SERVER,
                   "My logging message with special info: %d", 42);
 
    return UA_STATUSCODE_GOOD;
}
```

### Still unsure?
If any questions arise concerning code style, feel free to start an issue.
