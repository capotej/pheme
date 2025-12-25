5 Programming in C

This part of the manual explains the general concepts that you need to understand when interfacing to Guile from C. You will learn about how the latent typing of Scheme is embedded into the static typing of C, how the garbage collection of Guile is made available to C code, and how continuations influence the control flow in a C program.

This knowledge should make it straightforward to add new functions to Guile that can be called from Scheme. Adding new data types is also possible and is done by defining foreign objects.

The An Overview of Guile Programming section of this part contains general musings and guidelines about programming with Guile. It explores different ways to design a program around Guile, or how to embed Guile into existing programs.

For a pedagogical yet detailed explanation of how the data representation of Guile is implemented, See Data Representation. You don’t need to know the details given there to use Guile from C, but they are useful when you want to modify Guile itself or when you are just curious about how it is all done.

For detailed reference information on the variables, functions etc. that make up Guile’s application programming interface (API), See API Reference.

Parallel Installations
Linking Programs With Guile
Linking Guile with Libraries
General concepts for using libguile
Defining New Foreign Object Types
Function Snarfing
An Overview of Guile Programming
Autoconf Support
Next: Linking Programs With Guile, Up: Programming in C   [Contents][Index]

5.1 Parallel Installations

Guile provides strong API and ABI stability guarantees during stable series, so that if a user writes a program against Guile version 2.2.3, it will be compatible with some future version 2.2.7. We say in this case that 2.2 is the effective version, composed of the major and minor versions, in this case 2 and 2.

Users may install multiple effective versions of Guile, with each version’s headers, libraries, and Scheme files under their own directories. This provides the necessary stability guarantee for users, while also allowing Guile developers to evolve the language and its implementation.

However, parallel installability does have a down-side, in that users need to know which version of Guile to ask for, when they build against Guile. Guile solves this problem by installing a file to be read by the pkg-config utility, a tool to query installed packages by name. Guile encodes the version into its pkg-config name, so that users can ask for guile-2.2 or guile-3.0, as appropriate.

For effective version 3.0, for example, you would invoke pkg-config --cflags --libs guile-3.0 to get the compilation and linking flags necessary to link to version 3.0 of Guile. You would typically run pkg-config during the configuration phase of your program and use the obtained information in the Makefile.

Guile’s pkg-config file, guile-3.0.pc, defines additional useful variables:

sitedir
The default directory where Guile looks for Scheme source and compiled files (see %site-dir). Run pkg-config guile-3.0 --variable=sitedir to see its value. See GUILE_SITE_DIR, for more on how to use it from Autoconf.

extensiondir
The default directory where Guile looks for extensions—i.e., shared libraries providing additional features (see Foreign Extensions). Run pkg-config guile-3.0 --variable=extensiondir to see its value.

guile
guild
The absolute file name of the guile and guild commands4. Run pkg-config guile-3.0 --variable=guile or --variable=guild to see their value.

These variables allow users to deal with program name transformations that may be specified when configuring Guile with --program-transform-name, --program-suffix, or --program-prefix (see Transformation Options in GNU Autoconf Manual).

See the pkg-config man page, for more information, or its web site, http://pkg-config.freedesktop.org/. See Autoconf Support, for more on checking for Guile from within a configure.ac file.

Next: Linking Guile with Libraries, Previous: Parallel Installations, Up: Programming in C   [Contents][Index]

5.2 Linking Programs With Guile

This section covers the mechanics of linking your program with Guile on a typical POSIX system.

The header file <libguile.h> provides declarations for all of Guile’s functions and constants. You should #include it at the head of any C source file that uses identifiers described in this manual. Once you’ve compiled your source files, you need to link them against the Guile object code library, libguile.

As noted in the previous section, <libguile.h> is not in the default search path for headers. The following command lines give respectively the C compilation and link flags needed to build programs using Guile 3.0:

pkg-config guile-3.0 --cflags
pkg-config guile-3.0 --libs
Guile Initialization Functions
A Sample Guile Main Program
Building the Example with Make
Building the Example with Autoconf
Next: A Sample Guile Main Program, Up: Linking Programs With Guile   [Contents][Index]

5.2.1 Guile Initialization Functions

To initialize Guile, you can use one of several functions. The first, scm_with_guile, is the most portable way to initialize Guile. It will initialize Guile when necessary and then call a function that you can specify. Multiple threads can call scm_with_guile concurrently and it can also be called more than once in a given thread. The global state of Guile will survive from one call of scm_with_guile to the next. Your function is called from within scm_with_guile since the garbage collector of Guile needs to know where the stack of each thread is.

A second function, scm_init_guile, initializes Guile for the current thread. When it returns, you can use the Guile API in the current thread. This function employs some non-portable magic to learn about stack bounds and might thus not be available on all platforms.

One common way to use Guile is to write a set of C functions which perform some useful task, make them callable from Scheme, and then link the program with Guile. This yields a Scheme interpreter just like guile, but augmented with extra functions for some specific application — a special-purpose scripting language.

In this situation, the application should probably process its command-line arguments in the same manner as the stock Guile interpreter. To make that straightforward, Guile provides the scm_boot_guile and scm_shell function.

For more about these functions, see Initializing Guile.

Previous: Guile Initialization Functions, Up: Linking Programs With Guile   [Contents][Index]

5.2.2 A Sample Guile Main Program

Here is simple-guile.c, source code for a main and an inner_main function that will produce a complete Guile interpreter.

/* simple-guile.c --- Start Guile from C.  */

#include <libguile.h>

static void
inner_main (void *closure, int argc, char **argv)
{
  /* preparation */
  scm_shell (argc, argv);
  /* after exit */
}

int
main (int argc, char **argv)
{
  scm_boot_guile (argc, argv, inner_main, 0);
  return 0; /* never reached, see inner_main */
}
The main function calls scm_boot_guile to initialize Guile, passing it inner_main. Once scm_boot_guile is ready, it invokes inner_main, which calls scm_shell to process the command-line arguments in the usual way.

5.2.3 Building the Example with Make

Here is a Makefile which you can use to compile the example program. It uses pkg-config to learn about the necessary compiler and linker flags.

# Use GCC, if you have it installed.
CC=gcc

# Tell the C compiler where to find <libguile.h>
CFLAGS=`pkg-config --cflags guile-3.0`

# Tell the linker what libraries to use and where to find them.
LIBS=`pkg-config --libs guile-3.0`

simple-guile: simple-guile.o
        ${CC} simple-guile.o ${LIBS} -o simple-guile

simple-guile.o: simple-guile.c
        ${CC} -c ${CFLAGS} simple-guile.c
5.2.4 Building the Example with Autoconf

If you are using the GNU Autoconf package to make your application more portable, Autoconf will settle many of the details in the Makefile automatically, making it much simpler and more portable; we recommend using Autoconf with Guile. Here is a configure.ac file for simple-guile that uses the standard PKG_CHECK_MODULES macro to check for Guile. Autoconf will process this file into a configure script. We recommend invoking Autoconf via the autoreconf utility.

AC_INIT(simple-guile.c)

# Find a C compiler.
AC_PROG_CC

# Check for Guile
PKG_CHECK_MODULES([GUILE], [guile-3.0])

# Generate a Makefile, based on the results.
AC_OUTPUT(Makefile)
Run autoreconf -vif to generate configure.

Here is a Makefile.in template, from which the configure script produces a Makefile customized for the host system:

# The configure script fills in these values.
CC=@CC@
CFLAGS=@GUILE_CFLAGS@
LIBS=@GUILE_LIBS@

simple-guile: simple-guile.o
        ${CC} simple-guile.o ${LIBS} -o simple-guile
simple-guile.o: simple-guile.c
        ${CC} -c ${CFLAGS} simple-guile.c
The developer should use Autoconf to generate the configure script from the configure.ac template, and distribute configure with the application. Here’s how a user might go about building the application:

$ ls
Makefile.in     configure*      configure.ac    simple-guile.c
$ ./configure
checking for gcc... ccache gcc
checking whether the C compiler works... yes
checking for C compiler default output file name... a.out
checking for suffix of executables... 
checking whether we are cross compiling... no
checking for suffix of object files... o
checking whether we are using the GNU C compiler... yes
checking whether ccache gcc accepts -g... yes
checking for ccache gcc option to accept ISO C89... none needed
checking for pkg-config... /usr/bin/pkg-config
checking pkg-config is at least version 0.9.0... yes
checking for GUILE... yes
configure: creating ./config.status
config.status: creating Makefile
$ make
[...]
$ ./simple-guile
guile> (+ 1 2 3)
6
guile> (getpwnam "jimb")
#("jimb" "83Z7d75W2tyJQ" 4008 10 "Jim Blandy" "/u/jimb"
  "/usr/local/bin/bash")
guile> (exit)
$
Next: General concepts for using libguile, Previous: Linking Programs With Guile, Up: Programming in C   [Contents][Index]

5.3 Linking Guile with Libraries

The previous section has briefly explained how to write programs that make use of an embedded Guile interpreter. But sometimes, all you want to do is make new primitive procedures and data types available to the Scheme programmer. Writing a new version of guile is inconvenient in this case and it would in fact make the life of the users of your new features needlessly hard.

For example, suppose that there is a program guile-db that is a version of Guile with additional features for accessing a database. People who want to write Scheme programs that use these features would have to use guile-db instead of the usual guile program. Now suppose that there is also a program guile-gtk that extends Guile with access to the popular Gtk+ toolkit for graphical user interfaces. People who want to write GUIs in Scheme would have to use guile-gtk. Now, what happens when you want to write a Scheme application that uses a GUI to let the user access a database? You would have to write a third program that incorporates both the database stuff and the GUI stuff. This might not be easy (because guile-gtk might be a quite obscure program, say) and taking this example further makes it easy to see that this approach can not work in practice.

It would have been much better if both the database features and the GUI feature had been provided as libraries that can just be linked with guile. Guile makes it easy to do just this, and we encourage you to make your extensions to Guile available as libraries whenever possible.

You write the new primitive procedures and data types in the normal fashion, and link them into a shared library instead of into a stand-alone program. The shared library can then be loaded dynamically by Guile.

A Sample Guile Extension
Up: Linking Guile with Libraries   [Contents][Index]

5.3.1 A Sample Guile Extension

This section explains how to make the Bessel functions of the C library available to Scheme. First we need to write the appropriate glue code to convert the arguments and return values of the functions from Scheme to C and back. Additionally, we need a function that will add them to the set of Guile primitives. Because this is just an example, we will only implement this for the j0 function.

Consider the following file bessel.c.

#include <math.h>
#include <libguile.h>

SCM
j0_wrapper (SCM x)
{
  return scm_from_double (j0 (scm_to_double (x)));
}

void
init_bessel ()
{
  scm_c_define_gsubr ("j0", 1, 0, 0, j0_wrapper);
}
This C source file needs to be compiled into a shared library. Here is how to do it on GNU/Linux:

gcc `pkg-config --cflags guile-3.0` \
  -shared -o libguile-bessel.so -fPIC bessel.c
For creating shared libraries portably, we recommend the use of GNU Libtool (see Introduction in GNU Libtool).

A shared library can be loaded into a running Guile process with the function load-extension. In addition to the name of the library to load, this function also expects the name of a function from that library that will be called to initialize it. For our example, we are going to call the function init_bessel which will make j0_wrapper available to Scheme programs with the name j0. Note that we do not specify a filename extension such as .so when invoking load-extension. The right extension for the host platform will be provided automatically.

(load-extension "libguile-bessel" "init_bessel")
(j0 2)
⇒ 0.223890779141236
For this to work, load-extension must be able to find libguile-bessel, of course. It will look in the places that are usual for your operating system, and it will additionally look into the directories listed in the LTDL_LIBRARY_PATH environment variable.

To see how these Guile extensions via shared libraries relate to the module system, See Putting Extensions into Modules.

Next: Defining New Foreign Object Types, Previous: Linking Guile with Libraries, Up: Programming in C   [Contents][Index]

5.4 General concepts for using libguile

When you want to embed the Guile Scheme interpreter into your program or library, you need to link it against the libguile library (see Linking Programs With Guile). Once you have done this, your C code has access to a number of data types and functions that can be used to invoke the interpreter, or make new functions that you have written in C available to be called from Scheme code, among other things.

Scheme is different from C in a number of significant ways, and Guile tries to make the advantages of Scheme available to C as well. Thus, in addition to a Scheme interpreter, libguile also offers dynamic types, garbage collection, continuations, arithmetic on arbitrary sized numbers, and other things.

The two fundamental concepts are dynamic types and garbage collection. You need to understand how libguile offers them to C programs in order to use the rest of libguile. Also, the more general control flow of Scheme caused by continuations needs to be dealt with.

Running asynchronous signal handlers and multi-threading is known to C code already, but there are of course a few additional rules when using them together with libguile.

Dynamic Types
Garbage Collection
Control Flow
Asynchronous Signals
Multi-Threading
Next: Garbage Collection, Up: General concepts for using libguile   [Contents][Index]

5.4.1 Dynamic Types

Scheme is a dynamically-typed language; this means that the system cannot, in general, determine the type of a given expression at compile time. Types only become apparent at run time. Variables do not have fixed types; a variable may hold a pair at one point, an integer at the next, and a thousand-element vector later. Instead, values, not variables, have fixed types.

In order to implement standard Scheme functions like pair? and string? and provide garbage collection, the representation of every value must contain enough information to accurately determine its type at run time. Often, Scheme systems also use this information to determine whether a program has attempted to apply an operation to an inappropriately typed value (such as taking the car of a string).

Because variables, pairs, and vectors may hold values of any type, Scheme implementations use a uniform representation for values — a single type large enough to hold either a complete value or a pointer to a complete value, along with the necessary typing information.

In Guile, this uniform representation of all Scheme values is the C type SCM. This is an opaque type and its size is typically equivalent to that of a pointer to void. Thus, SCM values can be passed around efficiently and they take up reasonably little storage on their own.

The most important rule is: You never access a SCM value directly; you only pass it to functions or macros defined in libguile.

As an obvious example, although a SCM variable can contain integers, you can of course not compute the sum of two SCM values by adding them with the C + operator. You must use the libguile function scm_sum.

Less obvious and therefore more important to keep in mind is that you also cannot directly test SCM values for trueness. In Scheme, the value #f is considered false and of course a SCM variable can represent that value. But there is no guarantee that the SCM representation of #f looks false to C code as well. You need to use scm_is_true or scm_is_false to test a SCM value for trueness or falseness, respectively.

You also can not directly compare two SCM values to find out whether they are identical (that is, whether they are eq? in Scheme terms). You need to use scm_is_eq for this.

The one exception is that you can directly assign a SCM value to a SCM variable by using the C = operator.

The following (contrived) example shows how to do it right. It implements a function of two arguments (a and flag) that returns a+1 if flag is true, else it returns a unchanged.

SCM
my_incrementing_function (SCM a, SCM flag)
{
  SCM result;

  if (scm_is_true (flag))
    result = scm_sum (a, scm_from_int (1));
  else
    result = a;

  return result;
}
Often, you need to convert between SCM values and appropriate C values. For example, we needed to convert the integer 1 to its SCM representation in order to add it to a. Libguile provides many function to do these conversions, both from C to SCM and from SCM to C.

The conversion functions follow a common naming pattern: those that make a SCM value from a C value have names of the form scm_from_type (…) and those that convert a SCM value to a C value use the form scm_to_type (…).

However, it is best to avoid converting values when you can. When you must combine C values and SCM values in a computation, it is often better to convert the C values to SCM values and do the computation by using libguile functions than to the other way around (converting SCM to C and doing the computation some other way).

As a simple example, consider this version of my_incrementing_function from above:

SCM
my_other_incrementing_function (SCM a, SCM flag)
{
  int result;

  if (scm_is_true (flag))
    result = scm_to_int (a) + 1;
  else
    result = scm_to_int (a);

  return scm_from_int (result);
}
This version is much less general than the original one: it will only work for values A that can fit into a int. The original function will work for all values that Guile can represent and that scm_sum can understand, including integers bigger than long long, floating point numbers, complex numbers, and new numerical types that have been added to Guile by third-party libraries.

Also, computing with SCM is not necessarily inefficient. Small integers will be encoded directly in the SCM value, for example, and do not need any additional memory on the heap. See Data Representation to find out the details.

Some special SCM values are available to C code without needing to convert them from C values:

Scheme value	C representation
#f	SCM_BOOL_F
#t	SCM_BOOL_T
()	SCM_EOL
In addition to SCM, Guile also defines the related type scm_t_bits. This is an unsigned integral type of sufficient size to hold all information that is directly contained in a SCM value. The scm_t_bits type is used internally by Guile to do all the bit twiddling explained in Data Representation, but you will encounter it occasionally in low-level user code as well.

Next: Control Flow, Previous: Dynamic Types, Up: General concepts for using libguile   [Contents][Index]

5.4.2 Garbage Collection

As explained above, the SCM type can represent all Scheme values. Some values fit entirely into a SCM value (such as small integers), but other values require additional storage in the heap (such as strings and vectors). This additional storage is managed automatically by Guile. You don’t need to explicitly deallocate it when a SCM value is no longer used.

Two things must be guaranteed so that Guile is able to manage the storage automatically: it must know about all blocks of memory that have ever been allocated for Scheme values, and it must know about all Scheme values that are still being used. Given this knowledge, Guile can periodically free all blocks that have been allocated but are not used by any active Scheme values. This activity is called garbage collection.

Guile’s garbage collector will automatically discover references to SCM objects that originate in global variables, static data sections, function arguments or local variables on the C and Scheme stacks, and values in machine registers. Other references to SCM objects, such as those in other random data structures in the C heap that contain fields of type SCM, can be made visible to the garbage collector by calling the functions scm_gc_protect_object or scm_permanent_object. Collectively, these values form the “root set” of garbage collection; any value on the heap that is referenced directly or indirectly by a member of the root set is preserved, and all other objects are eligible for reclamation.

In Guile, garbage collection has two logical phases: the mark phase, in which the collector discovers the set of all live objects, and the sweep phase, in which the collector reclaims the resources associated with dead objects. The mark phase pauses the program and traces all SCM object references, starting with the root set. The sweep phase actually runs concurrently with the main program, incrementally reclaiming memory as needed by allocation.

In the mark phase, the garbage collector traces the Scheme stack and heap precisely. Because the Scheme stack and heap are managed by Guile, Guile can know precisely where in those data structures it might find references to other heap objects. This is not the case, unfortunately, for pointers on the C stack and static data segment. Instead of requiring the user to inform Guile about all variables in C that might point to heap objects, Guile traces the C stack and static data segment conservatively. That is to say, Guile just treats every word on the C stack and every C global variable as a potential reference in to the Scheme heap5. Any value that looks like a pointer to a GC-managed object is treated as such, whether it actually is a reference or not. Thus, scanning the C stack and static data segment is guaranteed to find all actual references, but it might also find words that only accidentally look like references. These “false positives” might keep SCM objects alive that would otherwise be considered dead. While this might waste memory, keeping an object around longer than it strictly needs to is harmless. This is why this technique is called “conservative garbage collection”. In practice, the wasted memory seems to be no problem, as the static C root set is almost always finite and small, given that the Scheme stack is separate from the C stack.

The stack of every thread is scanned in this way and the registers of the CPU and all other memory locations where local variables or function parameters might show up are included in this scan as well.

The consequence of the conservative scanning is that you can just declare local variables and function parameters of type SCM and be sure that the garbage collector will not free the corresponding objects.

However, a local variable or function parameter is only protected as long as it is really on the stack (or in some register). As an optimization, the C compiler might reuse its location for some other value and the SCM object would no longer be protected. Normally, this leads to exactly the right behavior: the compiler will only overwrite a reference when it is no longer needed and thus the object becomes unprotected precisely when the reference disappears, just as wanted.

There are situations, however, where a SCM object needs to be around longer than its reference from a local variable or function parameter. This happens, for example, when you retrieve some pointer from a foreign object and work with that pointer directly. The reference to the SCM foreign object might be dead after the pointer has been retrieved, but the pointer itself (and the memory pointed to) is still in use and thus the foreign object must be protected. The compiler does not know about this connection and might overwrite the SCM reference too early.

To get around this problem, you can use scm_remember_upto_here_1 and its cousins. It will keep the compiler from overwriting the reference. See Foreign Object Memory Management.

Next: Asynchronous Signals, Previous: Garbage Collection, Up: General concepts for using libguile   [Contents][Index]

5.4.3 Control Flow

Scheme has a more general view of program flow than C, both locally and non-locally.

Controlling the local flow of control involves things like gotos, loops, calling functions and returning from them. Non-local control flow refers to situations where the program jumps across one or more levels of function activations without using the normal call or return operations.

The primitive means of C for local control flow is the goto statement, together with if. Loops done with for, while or do could in principle be rewritten with just goto and if. In Scheme, the primitive means for local control flow is the function call (together with if). Thus, the repetition of some computation in a loop is ultimately implemented by a function that calls itself, that is, by recursion.

This approach is theoretically very powerful since it is easier to reason formally about recursion than about gotos. In C, using recursion exclusively would not be practical, though, since it would eat up the stack very quickly. In Scheme, however, it is practical: function calls that appear in a tail position do not use any additional stack space (see Tail calls).

A function call is in a tail position when it is the last thing the calling function does. The value returned by the called function is immediately returned from the calling function. In the following example, the call to bar-1 is in a tail position, while the call to bar-2 is not. (The call to 1- in foo-2 is in a tail position, though.)

(define (foo-1 x)
  (bar-1 (1- x)))

(define (foo-2 x)
  (1- (bar-2 x)))
Thus, when you take care to recurse only in tail positions, the recursion will only use constant stack space and will be as good as a loop constructed from gotos.

Scheme offers a few syntactic abstractions (do and named let) that make writing loops slightly easier.

But only Scheme functions can call other functions in a tail position: C functions can not. This matters when you have, say, two functions that call each other recursively to form a common loop. The following (unrealistic) example shows how one might go about determining whether a non-negative integer n is even or odd.

(define (my-even? n)
  (cond ((zero? n) #t)
        (else (my-odd? (1- n)))))

(define (my-odd? n)
  (cond ((zero? n) #f)
        (else (my-even? (1- n)))))
Because the calls to my-even? and my-odd? are in tail positions, these two procedures can be applied to arbitrary large integers without overflowing the stack. (They will still take a lot of time, of course.)

However, when one or both of the two procedures would be rewritten in C, it could no longer call its companion in a tail position (since C does not have this concept). You might need to take this consideration into account when deciding which parts of your program to write in Scheme and which in C.

In addition to calling functions and returning from them, a Scheme program can also exit non-locally from a function so that the control flow returns directly to an outer level. This means that some functions might not return at all.

Even more, it is not only possible to jump to some outer level of control, a Scheme program can also jump back into the middle of a function that has already exited. This might cause some functions to return more than once.

In general, these non-local jumps are done by invoking continuations that have previously been captured using call-with-current-continuation. Guile also offers a slightly restricted set of functions, catch and throw, that can only be used for non-local exits. This restriction makes them more efficient. Error reporting (with the function error) is implemented by invoking throw, for example. The functions catch and throw belong to the topic of exceptions.

Since Scheme functions can call C functions and vice versa, C code can experience the more general control flow of Scheme as well. It is possible that a C function will not return at all, or will return more than once. While C does offer setjmp and longjmp for non-local exits, it is still an unusual thing for C code. In contrast, non-local exits are very common in Scheme, mostly to report errors.

You need to be prepared for the non-local jumps in the control flow whenever you use a function from libguile: it is best to assume that any libguile function might signal an error or run a pending signal handler (which in turn can do arbitrary things).

It is often necessary to take cleanup actions when the control leaves a function non-locally. Also, when the control returns non-locally, some setup actions might be called for. For example, the Scheme function with-output-to-port needs to modify the global state so that current-output-port returns the port passed to with-output-to-port. The global output port needs to be reset to its previous value when with-output-to-port returns normally or when it is exited non-locally. Likewise, the port needs to be set again when control enters non-locally.

Scheme code can use the dynamic-wind function to arrange for the setting and resetting of the global state. C code can use the corresponding scm_internal_dynamic_wind function, or a scm_dynwind_begin/scm_dynwind_end pair together with suitable ’dynwind actions’ (see Dynamic Wind).

Instead of coping with non-local control flow, you can also prevent it by erecting a continuation barrier, See Continuation Barriers. The function scm_c_with_continuation_barrier, for example, is guaranteed to return exactly once.

Next: Multi-Threading, Previous: Control Flow, Up: General concepts for using libguile   [Contents][Index]

5.4.4 Asynchronous Signals

You can not call libguile functions from handlers for POSIX signals, but you can register Scheme handlers for POSIX signals such as SIGINT. These handlers do not run during the actual signal delivery. Instead, they are run when the program (more precisely, the thread that the handler has been registered for) reaches the next safe point.

The libguile functions themselves have many such safe points. Consequently, you must be prepared for arbitrary actions anytime you call a libguile function. For example, even scm_cons can contain a safe point and when a signal handler is pending for your thread, calling scm_cons will run this handler and anything might happen, including a non-local exit although scm_cons would not ordinarily do such a thing on its own.

If you do not want to allow the running of asynchronous signal handlers, you can block them temporarily with scm_dynwind_block_asyncs, for example. See Asynchronous Interrupts.

Since signal handling in Guile relies on safe points, you need to make sure that your functions do offer enough of them. Normally, calling libguile functions in the normal course of action is all that is needed. But when a thread might spent a long time in a code section that calls no libguile function, it is good to include explicit safe points. This can allow the user to interrupt your code with C-c, for example.

You can do this with the macro SCM_TICK. This macro is syntactically a statement. That is, you could use it like this:

while (1)
  {
    SCM_TICK;
    do_some_work ();
  }
Frequent execution of a safe point is even more important in multi threaded programs, See Multi-Threading.

Previous: Asynchronous Signals, Up: General concepts for using libguile   [Contents][Index]

5.4.5 Multi-Threading

Guile can be used in multi-threaded programs just as well as in single-threaded ones.

Each thread that wants to use functions from libguile must put itself into guile mode and must then follow a few rules. If it doesn’t want to honor these rules in certain situations, a thread can temporarily leave guile mode (but can no longer use libguile functions during that time, of course).

Threads enter guile mode by calling scm_with_guile, scm_boot_guile, or scm_init_guile. As explained in the reference documentation for these functions, Guile will then learn about the stack bounds of the thread and can protect the SCM values that are stored in local variables. When a thread puts itself into guile mode for the first time, it gets a Scheme representation and is listed by all-threads, for example.

Threads in guile mode can block (e.g., do blocking I/O) without causing any problems6; temporarily leaving guile mode with scm_without_guile before blocking slightly improves GC performance, though. For some common blocking operations, Guile provides convenience functions. For example, if you want to lock a pthread mutex while in guile mode, you might want to use scm_pthread_mutex_lock which is just like pthread_mutex_lock except that it leaves guile mode while blocking.

All libguile functions are (intended to be) robust in the face of multiple threads using them concurrently. This means that there is no risk of the internal data structures of libguile becoming corrupted in such a way that the process crashes.

A program might still produce nonsensical results, though. Taking hashtables as an example, Guile guarantees that you can use them from multiple threads concurrently and a hashtable will always remain a valid hashtable and Guile will not crash when you access it. It does not guarantee, however, that inserting into it concurrently from two threads will give useful results: only one insertion might actually happen, none might happen, or the table might in general be modified in a totally arbitrary manner. (It will still be a valid hashtable, but not the one that you might have expected.) Guile might also signal an error when it detects a harmful race condition.

Thus, you need to put in additional synchronizations when multiple threads want to use a single hashtable, or any other mutable Scheme object.

When writing C code for use with libguile, you should try to make it robust as well. An example that converts a list into a vector will help to illustrate. Here is a correct version:

SCM
my_list_to_vector (SCM list)
{
  SCM vector = scm_make_vector (scm_length (list), SCM_UNDEFINED);
  size_t len, i;

  len = scm_c_vector_length (vector);
  i = 0;
  while (i < len && scm_is_pair (list))
    {
      scm_c_vector_set_x (vector, i, scm_car (list));
      list = scm_cdr (list);
      i++;
    }

  return vector;
}
The first thing to note is that storing into a SCM location concurrently from multiple threads is guaranteed to be robust: you don’t know which value wins but it will in any case be a valid SCM value.

But there is no guarantee that the list referenced by list is not modified in another thread while the loop iterates over it. Thus, while copying its elements into the vector, the list might get longer or shorter. For this reason, the loop must check both that it doesn’t overrun the vector and that it doesn’t overrun the list. Otherwise, scm_c_vector_set_x would raise an error if the index is out of range, and scm_car and scm_cdr would raise an error if the value is not a pair.

It is safe to use scm_car and scm_cdr on the local variable list once it is known that the variable contains a pair. The contents of the pair might change spontaneously, but it will always stay a valid pair (and a local variable will of course not spontaneously point to a different Scheme object).

Likewise, a vector such as the one returned by scm_make_vector is guaranteed to always stay the same length so that it is safe to only use scm_c_vector_length once and store the result. (In the example, vector is safe anyway since it is a fresh object that no other thread can possibly know about until it is returned from my_list_to_vector.)

Of course the behavior of my_list_to_vector is suboptimal when list does indeed get asynchronously lengthened or shortened in another thread. But it is robust: it will always return a valid vector. That vector might be shorter than expected, or its last elements might be unspecified, but it is a valid vector and if a program wants to rule out these cases, it must avoid modifying the list asynchronously.

Here is another version that is also correct:

SCM
my_pedantic_list_to_vector (SCM list)
{
  SCM vector = scm_make_vector (scm_length (list), SCM_UNDEFINED);
  size_t len, i;

  len = scm_c_vector_length (vector);
  i = 0;
  while (i < len)
    {
      scm_c_vector_set_x (vector, i, scm_car (list));
      list = scm_cdr (list);
      i++;
    }

  return vector;
}
This version relies on the error-checking behavior of scm_car and scm_cdr. When the list is shortened (that is, when list holds a non-pair), scm_car will throw an error. This might be preferable to just returning a half-initialized vector.

The API for accessing vectors and arrays of various kinds from C takes a slightly different approach to thread-robustness. In order to get at the raw memory that stores the elements of an array, you need to reserve that array as long as you need the raw memory. During the time an array is reserved, its elements can still spontaneously change their values, but the memory itself and other things like the size of the array are guaranteed to stay fixed. Any operation that would change these parameters of an array that is currently reserved will signal an error. In order to avoid these errors, a program should of course put suitable synchronization mechanisms in place. As you can see, Guile itself is again only concerned about robustness, not about correctness: without proper synchronization, your program will likely not be correct, but the worst consequence is an error message.

Real thread-safety often requires that a critical section of code is executed in a certain restricted manner. A common requirement is that the code section is not entered a second time when it is already being executed. Locking a mutex while in that section ensures that no other thread will start executing it, blocking asyncs ensures that no asynchronous code enters the section again from the current thread, and the error checking of Guile mutexes guarantees that an error is signaled when the current thread accidentally reenters the critical section via recursive function calls.

Guile provides two mechanisms to support critical sections as outlined above. You can either use the macros SCM_CRITICAL_SECTION_START and SCM_CRITICAL_SECTION_END for very simple sections; or use a dynwind context together with a call to scm_dynwind_critical_section.

The macros only work reliably for critical sections that are guaranteed to not cause a non-local exit. They also do not detect an accidental reentry by the current thread. Thus, you should probably only use them to delimit critical sections that do not contain calls to libguile functions or to other external functions that might do complicated things.

The function scm_dynwind_critical_section, on the other hand, will correctly deal with non-local exits because it requires a dynwind context. Also, by using a separate mutex for each critical section, it can detect accidental reentries.

Next: Function Snarfing, Previous: General concepts for using libguile, Up: Programming in C   [Contents][Index]

5.5 Defining New Foreign Object Types

The foreign object type facility is Guile’s mechanism for importing object and types from C or other languages into Guile’s system. If you have a C struct foo type, for example, you can define a corresponding Guile foreign object type that allows Scheme code to handle struct foo * objects.

To define a new foreign object type, the programmer provides Guile with some essential information about the type — what its name is, how many fields it has, and its finalizer (if any) — and Guile allocates a fresh type for it. Foreign objects can be accessed from Scheme or from C.

Defining Foreign Object Types
Creating Foreign Objects
Type Checking of Foreign Objects
Foreign Object Memory Management
Foreign Objects and Scheme
Next: Creating Foreign Objects, Up: Defining New Foreign Object Types   [Contents][Index]

5.5.1 Defining Foreign Object Types

To create a new foreign object type from C, call scm_make_foreign_object_type. It returns a value of type SCM which identifies the new type.

Here is how one might declare a new type representing eight-bit gray-scale images:

#include <libguile.h>

struct image {
  int width, height;
  char *pixels;

  /* The name of this image */
  SCM name;

  /* A function to call when this image is
     modified, e.g., to update the screen,
     or SCM_BOOL_F if no action necessary */
  SCM update_func;
};

static SCM image_type;

void
init_image_type (void)
{
  SCM name, slots;
  scm_t_struct_finalize finalizer;

  name = scm_from_utf8_symbol ("image");
  slots = scm_list_1 (scm_from_utf8_symbol ("data"));
  finalizer = NULL;

  image_type =
    scm_make_foreign_object_type (name, slots, finalizer);
}
The result is an initialized image_type value that identifies the new foreign object type. The next section describes how to create foreign objects and how to access their slots.

Next: Type Checking of Foreign Objects, Previous: Defining Foreign Object Types, Up: Defining New Foreign Object Types   [Contents][Index]

5.5.2 Creating Foreign Objects

Foreign objects contain zero or more “slots” of data. A slot can hold a pointer, an integer that fits into a size_t or ssize_t, or a SCM value.

All objects of a given foreign type have the same number of slots. In the example from the previous section, the image type has one slot, because the slots list passed to scm_make_foreign_object_type is of length one. (The actual names given to slots are unimportant for most users of the C interface, but can be used on the Scheme side to introspect on the foreign object.)

To construct a foreign object and initialize its first slot, call scm_make_foreign_object_1 (type, first_slot_value). There are similarly named constructors for initializing 0, 1, 2, or 3 slots, or initializing n slots via an array. See Foreign Objects, for full details. Any fields that are not explicitly initialized are set to 0.

To get or set the value of a slot by index, you can use the scm_foreign_object_ref and scm_foreign_object_set_x functions. These functions take and return values as void * pointers; there are corresponding convenience procedures like _signed_ref, _unsigned_set_x and so on for dealing with slots as signed or unsigned integers.

Foreign objects fields that are pointers can be tricky to manage. If possible, it is best that all memory that is referenced by a foreign object be managed by the garbage collector. That way, the GC can automatically ensure that memory is accessible when it is needed, and freed when it becomes inaccessible. If this is not the case for your program – for example, if you are exposing an object to Scheme that was allocated by some other, Guile-unaware part of your program – then you will probably need to implement a finalizer. See Foreign Object Memory Management, for more.

Continuing the example from the previous section, if the global variable image_type contains the type returned by scm_make_foreign_object_type, here is how we could construct a foreign object whose “data” field contains a pointer to a freshly allocated struct image:

SCM
make_image (SCM name, SCM s_width, SCM s_height)
{
  struct image *image;
  int width = scm_to_int (s_width);
  int height = scm_to_int (s_height);

  /* Allocate the `struct image'.  Because we
     use scm_gc_malloc, this memory block will
     be automatically reclaimed when it becomes
     inaccessible, and its members will be traced
     by the garbage collector.  */
  image = (struct image *)
    scm_gc_malloc (sizeof (struct image), "image");

  image->width = width;
  image->height = height;

  /* Allocating the pixels with
     scm_gc_malloc_pointerless means that the
     pixels data is collectable by GC, but
     that GC shouldn't spend time tracing its
     contents for nested pointers because there
     aren't any.  */
  image->pixels =
    scm_gc_malloc_pointerless (width * height, "image pixels");

  image->name = name;
  image->update_func = SCM_BOOL_F;

  /* Now wrap the struct image* in a new foreign
     object, and return that object.  */
  return scm_make_foreign_object_1 (image_type, image);
}
We use scm_gc_malloc_pointerless for the pixel buffer to tell the garbage collector not to scan it for pointers. Calls to scm_gc_malloc, scm_make_foreign_object_1, and scm_gc_malloc_pointerless raise an exception in out-of-memory conditions; the garbage collector is able to reclaim previously allocated memory if that happens.

Next: Foreign Object Memory Management, Previous: Creating Foreign Objects, Up: Defining New Foreign Object Types   [Contents][Index]

5.5.3 Type Checking of Foreign Objects

Functions that operate on foreign objects should check that the passed SCM value indeed is of the correct type before accessing its data. They can do this with scm_assert_foreign_object_type.

For example, here is a simple function that operates on an image object, and checks the type of its argument.

SCM
clear_image (SCM image_obj)
{
  int area;
  struct image *image;

  scm_assert_foreign_object_type (image_type, image_obj);

  image = scm_foreign_object_ref (image_obj, 0);
  area = image->width * image->height;
  memset (image->pixels, 0, area);

  /* Invoke the image's update function.  */
  if (scm_is_true (image->update_func))
    scm_call_0 (image->update_func);

  return SCM_UNSPECIFIED;
}
Next: Foreign Objects and Scheme, Previous: Type Checking of Foreign Objects, Up: Defining New Foreign Object Types   [Contents][Index]

5.5.4 Foreign Object Memory Management

Once a foreign object has been released to the tender mercies of the Scheme system, it must be prepared to survive garbage collection. In the example above, all the memory associated with the foreign object is managed by the garbage collector because we used the scm_gc_ allocation functions. Thus, no special care must be taken: the garbage collector automatically scans them and reclaims any unused memory.

However, when data associated with a foreign object is managed in some other way—e.g., malloc’d memory or file descriptors—it is possible to specify a finalizer function to release those resources when the foreign object is reclaimed.

As discussed in see Garbage Collection, Guile’s garbage collector will reclaim inaccessible memory as needed. This reclamation process runs concurrently with the main program. When Guile analyzes the heap and determines that an object’s memory can be reclaimed, that memory is put on a “free list” of objects that can be reclaimed. Usually that’s the end of it—the object is available for immediate re-use. However some objects can have “finalizers” associated with them—functions that are called on reclaimable objects to effect any external cleanup actions.

Finalizers are tricky business and it is best to avoid them. They can be invoked at unexpected times, or not at all—for example, they are not invoked on process exit. They don’t help the garbage collector do its job; in fact, they are a hindrance. Furthermore, they perturb the garbage collector’s internal accounting. The GC decides to scan the heap when it thinks that it is necessary, after some amount of allocation. Finalizable objects almost always represent an amount of allocation that is invisible to the garbage collector. The effect can be that the actual resource usage of a system with finalizable objects is higher than what the GC thinks it should be.

All those caveats aside, some foreign object types will need finalizers. For example, if we had a foreign object type that wrapped file descriptors—and we aren’t suggesting this, as Guile already has ports —then you might define the type like this:

static SCM file_type;

static void
finalize_file (SCM file)
{
  int fd = scm_foreign_object_signed_ref (file, 0);
  if (fd >= 0)
    {
      scm_foreign_object_signed_set_x (file, 0, -1);
      close (fd);
    }
}

static void
init_file_type (void)
{
  SCM name, slots;
  scm_t_struct_finalize finalizer;

  name = scm_from_utf8_symbol ("file");
  slots = scm_list_1 (scm_from_utf8_symbol ("fd"));
  finalizer = finalize_file;

  image_type =
    scm_make_foreign_object_type (name, slots, finalizer);
}

static SCM
make_file (int fd)
{
  return scm_make_foreign_object_1 (file_type, (void *) fd);
}
Note that the finalizer may be invoked in ways and at times you might not expect. In a Guile built without threading support, finalizers are invoked via “asyncs”, which interleaves them with running Scheme code; see Asynchronous Interrupts. If the user’s Guile is built with support for threads, the finalizer will probably be called by a dedicated finalization thread, unless the user invokes scm_run_finalizers () explicitly.

In either case, finalizers run concurrently with the main program, and so they need to be async-safe and thread-safe. If for some reason this is impossible, perhaps because you are embedding Guile in some application that is not itself thread-safe, you have a few options. One is to use guardians instead of finalizers, and arrange to pump the guardians for finalizable objects. See Guardians, for more information. The other option is to disable automatic finalization entirely, and arrange to call scm_run_finalizers () at appropriate points. See Foreign Objects, for more on these interfaces.

Finalizers are allowed to allocate memory, access GC-managed memory, and in general can do anything any Guile user code can do. This was not the case in Guile 1.8, where finalizers were much more restricted. In particular, in Guile 2.0, finalizers can resuscitate objects. We do not recommend that users avail themselves of this possibility, however, as a resuscitated object can re-expose other finalizable objects that have been already finalized back to Scheme. These objects will not be finalized again, but they could cause use-after-free problems to code that handles objects of that particular foreign object type. To guard against this possibility, robust finalization routines should clear state from the foreign object, as in the above free_file example.

One final caveat. Foreign object finalizers are associated with the lifetime of a foreign object, not of its fields. If you access a field of a finalizable foreign object, and do not arrange to keep a reference on the foreign object itself, it could be that the outer foreign object gets finalized while you are working with its field.

For example, consider a procedure to read some data from a file, from our example above.

SCM
read_bytes (SCM file, SCM n)
{
  int fd;
  SCM buf;
  size_t len, pos;

  scm_assert_foreign_object_type (file_type, file);

  fd = scm_foreign_object_signed_ref (file, 0);
  if (fd < 0)
    scm_wrong_type_arg_msg ("read-bytes", SCM_ARG1,
                            file, "open file");

  len = scm_to_size_t (n);
  SCM buf = scm_c_make_bytevector (scm_to_size_t (n));

  pos = 0;
  while (pos < len)
    {
      char *bytes = SCM_BYTEVECTOR_CONTENTS (buf);
      ssize_t count = read (fd, bytes + pos, len - pos);
      if (count < 0)
        scm_syserror ("read-bytes");
      if (count == 0)
        break;
      pos += count;
    }

  scm_remember_upto_here_1 (file);

  return scm_values (scm_list_2 (buf, scm_from_size_t (pos)));
}
After the prelude, only the fd value is used and the C compiler has no reason to keep the file object around. If scm_c_make_bytevector results in a garbage collection, file might not be on the stack or anywhere else and could be finalized, leaving read to read a closed (or, in a multi-threaded program, possibly re-used) file descriptor. The use of scm_remember_upto_here_1 prevents this, by creating a reference to file after all data accesses. See Function related to Garbage Collection.

scm_remember_upto_here_1 is only needed on finalizable objects, because garbage collection of other values is invisible to the program – it happens when needed, and is not observable. But if you can, save yourself the headache and build your program in such a way that it doesn’t need finalization.

Previous: Foreign Object Memory Management, Up: Defining New Foreign Object Types   [Contents][Index]

5.5.5 Foreign Objects and Scheme

It is also possible to create foreign objects and object types from Scheme, and to access fields of foreign objects from Scheme. For example, the file example from the last section could be equivalently expressed as:

(define-module (my-file)
  #:use-module (system foreign-object)
  #:use-module ((oop goops) #:select (make))
  #:export (make-file))

(define (finalize-file file)
  (let ((fd (struct-ref file 0)))
    (unless (< fd 0)
      (struct-set! file 0 -1)
      (close-fdes fd))))

(define <file>
  (make-foreign-object-type '<file> '(fd)
                            #:finalizer finalize-file))

(define (make-file fd)
  (make <file> #:fd fd))
Here we see that the result of make-foreign-object-type, which is the equivalent of scm_make_foreign_object_type, is a struct vtable. See Vtables, for more information. To instantiate the foreign object, which is really a Guile struct, we use make. (We could have used make-struct/no-tail, but as an implementation detail, finalizers are attached in the initialize method called by make). To access the fields, we use struct-ref and struct-set!. See Structure Basics.

There is a convenience syntax, define-foreign-object-type, that defines a type along with a constructor, and getters for the fields. An appropriate invocation of define-foreign-object-type for the file object type could look like this:

(use-modules (system foreign-object))

(define-foreign-object-type <file>
  make-file
  (fd)
  #:finalizer finalize-file)
This defines the <file> type with one field, a make-file constructor, and a getter for the fd field, bound to fd.

Foreign object types are not only vtables but are actually GOOPS classes, as hinted at above. See GOOPS, for more on Guile’s object-oriented programming system. Thus one can define print and equality methods using GOOPS:

(use-modules (oop goops))

(define-method (write (file <file>) port)
  ;; Assuming existence of the `fd' getter
  (format port "#<<file> ~a>" (fd file)))

(define-method (equal? (a <file>) (b <file>))
  (eqv? (fd a) (fd b)))
One can even sub-class foreign types.

(define-class <named-file> (<file>)
  (name #:init-keyword #:name #:init-value #f #:accessor name))
The question arises of how to construct these values, given that make-file returns a plain old <file> object. It turns out that you can use the GOOPS construction interface, where every field of the foreign object has an associated initialization keyword argument.

(define* (my-open-file name #:optional (flags O_RDONLY))
  (make <named-file> #:fd (open-fdes name flags) #:name name))

(define-method (write (file <named-file>) port)
  (format port "#<<file> ~s ~a>" (name file) (fd file)))
See Foreign Objects, for full documentation on the Scheme interface to foreign objects. See GOOPS, for more on GOOPS.

As a final note, you might wonder how this system supports encapsulation of sensitive values. First, we have to recognize that some facilities are essentially unsafe and have global scope. For example, in C, the integrity and confidentiality of a part of a program is at the mercy of every other part of that program – because any part of the program can read and write anything in its address space. At the same time, principled access to structured data is organized in C on lexical boundaries; if you don’t expose accessors for your object, you trust other parts of the program not to work around that barrier.

The situation is not dissimilar in Scheme. Although Scheme’s unsafe constructs are fewer in number than in C, they do exist. The (system foreign) module can be used to violate confidentiality and integrity, and shouldn’t be exposed to untrusted code. Although struct-ref and struct-set! are less unsafe, they still have a cross-cutting capability of drilling through abstractions. Performing a struct-set! on a foreign object slot could cause unsafe foreign code to crash. Ultimately, structures in Scheme are capabilities for abstraction, and not abstractions themselves.

That leaves us with the lexical capabilities, like constructors and accessors. Here is where encapsulation lies: the practical degree to which the innards of your foreign objects are exposed is the degree to which their accessors are lexically available in user code. If you want to allow users to reference fields of your foreign object, provide them with a getter. Otherwise you should assume that the only access to your object may come from your code, which has the relevant authority, or via code with access to cross-cutting struct-ref and such, which also has the cross-cutting authority.

Next: An Overview of Guile Programming, Previous: Defining New Foreign Object Types, Up: Programming in C   [Contents][Index]

5.6 Function Snarfing

When writing C code for use with Guile, you typically define a set of C functions, and then make some of them visible to the Scheme world by calling scm_c_define_gsubr or related functions. If you have many functions to publish, it can sometimes be annoying to keep the list of calls to scm_c_define_gsubr in sync with the list of function definitions.

Guile provides the guile-snarf program to manage this problem. Using this tool, you can keep all the information needed to define the function alongside the function definition itself; guile-snarf will extract this information from your source code, and automatically generate a file of calls to scm_c_define_gsubr which you can #include into an initialization function.

The snarfing mechanism works for many kind of initialization actions, not just for collecting calls to scm_c_define_gsubr. For a full list of what can be done, See Snarfing Macros.

The guile-snarf program is invoked like this:

guile-snarf [-o outfile] [cpp-args ...]
This command will extract initialization actions to outfile. When no outfile has been specified or when outfile is -, standard output will be used. The C preprocessor is called with cpp-args (which usually include an input file) and the output is filtered to extract the initialization actions.

If there are errors during processing, outfile is deleted and the program exits with non-zero status.

During snarfing, the pre-processor macro SCM_MAGIC_SNARFER is defined. You could use this to avoid including snarfer output files that don’t yet exist by writing code like this:

#ifndef SCM_MAGIC_SNARFER
#include "foo.x"
#endif
Here is how you might define the Scheme function clear-image, implemented by the C function clear_image:

#include <libguile.h>

SCM_DEFINE (clear_image, "clear-image", 1, 0, 0,
            (SCM image),
            "Clear the image.")
{
  /* C code to clear the image in image... */
}

void
init_image_type ()
{
#include "image-type.x"
}
The SCM_DEFINE declaration says that the C function clear_image implements a Scheme function called clear-image, which takes one required argument (of type SCM and named image), no optional arguments, and no rest argument. The string "Clear the image." provides a short help text for the function, it is called a docstring.

SCM_DEFINE macro also defines a static array of characters initialized to the Scheme name of the function. In this case, s_clear_image is set to the C string, "clear-image". You might want to use this symbol when generating error messages.

Assuming the text above lives in a file named image-type.c, you will need to execute the following command to prepare this file for compilation:

guile-snarf -o image-type.x image-type.c
This scans image-type.c for SCM_DEFINE declarations, and writes to image-type.x the output:

scm_c_define_gsubr ("clear-image", 1, 0, 0, (SCM (*)() ) clear_image);
When compiled normally, SCM_DEFINE is a macro which expands to the function header for clear_image.

Note that the output file name matches the #include from the input file. Also, you still need to provide all the same information you would if you were using scm_c_define_gsubr yourself, but you can place the information near the function definition itself, so it is less likely to become incorrect or out-of-date.

If you have many files that guile-snarf must process, you should consider using a fragment like the following in your Makefile:

snarfcppopts = $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
.SUFFIXES: .x
.c.x:
	guile-snarf -o $@ $< $(snarfcppopts)
This tells make to run guile-snarf to produce each needed .x file from the corresponding .c file.

The program guile-snarf passes its command-line arguments directly to the C preprocessor, which it uses to extract the information it needs from the source code. this means you can pass normal compilation flags to guile-snarf to define preprocessor symbols, add header file directories, and so on.