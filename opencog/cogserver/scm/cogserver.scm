;
; Opencog cogserver module
;

(define-module (opencog cogserver))

(use-modules (opencog))
(use-modules (opencog cs-config))

; Load the C library that calls the classserver to load the types.
(load-extension
	(string-append opencog-ext-path-cogserver-types "libcogserver-types")
	"cogserver_types_init")

; Load the cogserver types scheme bindings (e.g. CogServerNode)
; Use include-from-path so that define-public exports from this module.
(include-from-path "opencog/cogserver/types/cogserver_types.scm")

; Path to libguile-cogserver.so is set up in the cs-config module.
(load-extension
  (string-append opencog-ext-path-cogserver "libguile-cogserver")
  "opencog_cogserver_init")

; config path name is optional.
(define* (start-cogserver #:key (port 17001)
                                (web  18080)
                                (mcp  18888)
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

  Start the cogserver, optionally specifying a telnet port, websocket
  port, telnet prompt and telnet scheme prompt (or any combination of
  these). If any are missing, default values will be used. The defaults
  are as shown.

  If any of the ports ares set to zero, then the corresponding server
  will not be started.  At least one must be non-zero.

  The prompts may be ANSI colorized prompts. The ANSI escape sequence
  uses the ESC char, which is written as \\x1b in guile.

  To stop the cogserver, just say stop-cogserver.
"
	(if (string? port) (set! port (string->number port)))

	(define csn (CogServerNode "cogserver"))
	(cog-set-value! csn (Predicate "*-telnet-port-*") (FloatValue port))
	(cog-set-value! csn (Predicate "*-web-port-*") (FloatValue web))
	(cog-set-value! csn (Predicate "*-mcp-port-*") (FloatValue mcp))
	(cog-set-value! csn (Predicate "*-ansi-enabled-*") (BoolValue #t))
	(cog-set-value! csn (Predicate "*-ansi-prompt-*") (StringValue prompt))
	(cog-set-value! csn (Predicate "*-ansi-scm-prompt-*") (StringValue scmprompt))
	(cog-set-value! csn (Predicate "*-start-*") (VoidValue))
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
	; so we stop that, here ... XXX what?? where ?
	;   (stop-server-and-clients!)  ???
	(define csn (CogServerNode "cogserver"))
	(cog-set-value! csn (Predicate "*-stop-*") (VoidValue))
)

(export start-cogserver)
(export stop-cogserver)
