;
; Opencog cogserver module
;

(define-module (opencog cogserver))

(use-modules (opencog))
(use-modules (opencog cs-config))
(use-modules (opencog logger))

; Path to libguile-cogserver.so is set up in the cs-config module.
(load-extension
	(string-append opencog-ext-path-cogserver "libguile-cogserver")
	"opencog_cogserver_init")

(export get-cogserver-atomspace)
(export set-cogserver-atomspace!)

; config path name is optional.
(define* (start-cogserver #:optional (config-path "")
                          #:key (port 17001)
                                (web  18080)
                                (mcp  18888)
                                (logfile   "/tmp/cogserver.log")
                                (prompt    "[0;32mopencog[1;32m> [0m")
                                (scmprompt "[0;34mguile[1;34m> [0m"))
"
  start-cogserver
  start-cogserver #:port 17001
  start-cogserver #:web 18080
  start-cogserver #:mcp 18888
  start-cogserver #:logfile \"/tmp/cogserver.log\"
  start-cogserver #:prompt \"[0;32mopencog[1;32m> [0m\"
  start-cogserver #:prompt \"\\x1b[0;32mopencog\\x1b[1;32m> \\x1b[0m\"
  start-cogserver #:scmprompt \"[0;34mguile[1;34m> [0m\"
  start-cogserver #:scmprompt \"\\x1b[0;34mguile\\x1b[1;34m> \\x1b[0m\"
  start-cogserver [config-file]

  Start the cogserver, optionally specifying a telnet port, websocket
  port, logfile, telnet prompt and telnet scheme prompt (or any
  combination of these). If any are missing, default values will be
  used. The defaults are as shown.  Additional logging options can
  be found in the (opencog logger) module.

  Alternately, a config file can be specified. If a config file is
  given, as well as the above parameters, then the above parameters take
  precedence.  The use of a config file is deprecated.

  If either port is set to zero, then that server will not be started.
  At least one of the two must be non-zero.

  The prompts may be ANSI colorized prompts. The ANSI escape sequence
  uses the ESC char, which is written as \\x1b in guile.

  To stop the cogserver, just say stop-cogserver.
"
	(cog-logger-set-filename! logfile)
	(if (string? port) (set! port (string->number port)))
	(c-start-cogserver (cog-atomspace) port web mcp prompt scmprompt config-path)
)

; To stop the repl server..
(use-modules (system repl server))

; Similar to above
(define (stop-cogserver)
"
  stop-cogserver

  Stop the cogserver.

  See also: start-cogserver
"
	; The start-cogserver also starts a repl shell on port 18001
	; so we stop that, here ...
	(stop-server-and-clients!)
	(c-stop-cogserver)
)

(set-procedure-property! get-cogserver-atomspace 'documentation
"
 get-cogserver-atomspace

   Return the default AtomSpace of the cogserver.

   The default AtomSpace is the one that is used whenever a new shell
   is opened on an existing or new network connection.

   See also: set-cogserver-atomspace!
")

(set-procedure-property! set-cogserver-atomspace! 'documentation
"
 set-cogserver-atomspace! ATOMSPACE

   Change the default AtomSpace of the cogserver to ATOMSPACE.

   The default AtomSpace is the one that is used whenever a new shell
   is opened on an existing or new network connection. Changing this
   will not affect existing open shells.  Existing shells can get the
   new AtomSpace by saying `(cog-set-atomspace! (get-server-atomspace))`

   This call is useful when a stack of AtomSpace frames is being used,
   and there is a need for setting the active frame.

   Returns the old AtomSpace.
")

(export start-cogserver)
(export stop-cogserver)
