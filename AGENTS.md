6.4 Initializing Guile
======================

Each thread that wants to use functions from the Guile API needs to put
itself into guile mode with either â€˜scm_with_guileâ€™ or â€˜scm_init_guileâ€™.
The global state of Guile is initialized automatically when the first
thread enters guile mode.

   When a thread wants to block outside of a Guile API function, it
should leave guile mode temporarily with â€˜scm_without_guileâ€™, *Note
Blocking::.

   Threads that are created by â€˜call-with-new-threadâ€™ or
â€˜scm_spawn_threadâ€™ start out in guile mode so you donâ€™t need to
initialize them.

 -- C Function: void * scm_with_guile (void *(*func)(void *), void
          *data)
     Call FUNC, passing it DATA and return what FUNC returns.  While
     FUNC is running, the current thread is in guile mode and can thus
     use the Guile API.

     When â€˜scm_with_guileâ€™ is called from guile mode, the thread remains
     in guile mode when â€˜scm_with_guileâ€™ returns.

     Otherwise, it puts the current thread into guile mode and, if
     needed, gives it a Scheme representation that is contained in the
     list returned by â€˜all-threadsâ€™, for example.  This Scheme
     representation is not removed when â€˜scm_with_guileâ€™ returns so that
     a given thread is always represented by the same Scheme value
     during its lifetime, if at all.

     When this is the first thread that enters guile mode, the global
     state of Guile is initialized before calling â€˜funcâ€™.

     The function FUNC is called via â€˜scm_with_continuation_barrierâ€™;
     thus, â€˜scm_with_guileâ€™ returns exactly once.

     When â€˜scm_with_guileâ€™ returns, the thread is no longer in guile
     mode (except when â€˜scm_with_guileâ€™ was called from guile mode, see
     above).  Thus, only â€˜funcâ€™ can store â€˜SCMâ€™ variables on the stack
     and be sure that they are protected from the garbage collector.
     See â€˜scm_init_guileâ€™ for another approach at initializing Guile
     that does not have this restriction.

     It is OK to call â€˜scm_with_guileâ€™ while a thread has temporarily
     left guile mode via â€˜scm_without_guileâ€™.  It will then simply
     temporarily enter guile mode again.

 -- C Function: void scm_init_guile ()
     Arrange things so that all of the code in the current thread
     executes as if from within a call to â€˜scm_with_guileâ€™.  That is,
     all functions called by the current thread can assume that â€˜SCMâ€™
     values on their stack frames are protected from the garbage
     collector (except when the thread has explicitly left guile mode,
     of course).

     When â€˜scm_init_guileâ€™ is called from a thread that already has been
     in guile mode once, nothing happens.  This behavior matters when
     you call â€˜scm_init_guileâ€™ while the thread has only temporarily
     left guile mode: in that case the thread will not be in guile mode
     after â€˜scm_init_guileâ€™ returns.  Thus, you should not use
     â€˜scm_init_guileâ€™ in such a scenario.

     When a uncaught throw happens in a thread that has been put into
     guile mode via â€˜scm_init_guileâ€™, a short message is printed to the
     current error port and the thread is exited via â€˜scm_pthread_exit
     (NULL)â€™.  No restrictions are placed on continuations.

     The function â€˜scm_init_guileâ€™ might not be available on all
     platforms since it requires some stack-bounds-finding magic that
     might not have been ported to all platforms that Guile runs on.
     Thus, if you can, it is better to use â€˜scm_with_guileâ€™ or its
     variation â€˜scm_boot_guileâ€™ instead of this function.

 -- C Function: void scm_boot_guile (int ARGC, char **ARGV, void
          (*MAIN_FUNC) (void *DATA, int ARGC, char **ARGV), void *DATA)
     Enter guile mode as with â€˜scm_with_guileâ€™ and call MAIN_FUNC,
     passing it DATA, ARGC, and ARGV as indicated.  When MAIN_FUNC
     returns, â€˜scm_boot_guileâ€™ calls â€˜exit (0)â€™; â€˜scm_boot_guileâ€™ never
     returns.  If you want some other exit value, have MAIN_FUNC call
     â€˜exitâ€™ itself.  If you donâ€™t want to exit at all, use
     â€˜scm_with_guileâ€™ instead of â€˜scm_boot_guileâ€™.

     The function â€˜scm_boot_guileâ€™ arranges for the Scheme
     â€˜command-lineâ€™ function to return the strings given by ARGC and
     ARGV.  If MAIN_FUNC modifies ARGC or ARGV, it should call
     â€˜scm_set_program_argumentsâ€™ with the final list, so Scheme code
     will know which arguments have been processed (*note Runtime
     Environment::).

 -- C Function: void scm_shell (int ARGC, char **ARGV)
     Process command-line arguments in the manner of the â€˜guileâ€™
     executable.  This includes loading the normal Guile initialization
     files, interacting with the user or running any scripts or
     expressions specified by â€˜-sâ€™ or â€˜-eâ€™ options, and then exiting.
     *Note Invoking Guile::, for more details.

     Since this function does not return, you must do all
     application-specific initialization before calling this function.