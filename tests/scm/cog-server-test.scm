#! /usr/bin/env -S guile -s
!#
;
; cog-server-test.scm -- Test that the cogserver module loads and works
;
(use-modules (opencog))
(use-modules (opencog test-runner))
(use-modules (opencog cogserver))
(use-modules (ice-9 rdelim))

(opencog-test-runner)
(define tname "cog-server-test")
(test-begin tname)

;; -------------------------------------------------------
;; Helper function to test if a port can be connected to
(define (can-connect-to-port? port)
   (catch #t
      (lambda ()
         (let ((sock (socket AF_INET SOCK_STREAM 0)))
            (connect sock AF_INET INADDR_LOOPBACK port)
            (close-port sock)
            #t))
      (lambda (key . args)
         #f)))

;; Helper to send data and receive response from a port
(define (send-recv port data)
   (let ((sock (socket AF_INET SOCK_STREAM 0)))
      (connect sock AF_INET INADDR_LOOPBACK port)
      (display data sock)
      (force-output sock)
      ;; Shutdown write side to signal we're done sending
      (shutdown sock 1)
      (let loop ((result ""))
         (let ((line (read-line sock)))
            (if (eof-object? line)
               (begin (close-port sock) result)
               (loop (string-append result line "\n")))))))

;; -------------------------------------------------------
;; ===== Test the exported start-cogserver and stop-cogserver functions =====
(define exported-telnet 17401)
(define exported-web 18481)

(define exported-csn (start-cogserver #:port exported-telnet #:web exported-web #:mcp 0))

(test-assert "start-cogserver"
   (cog-atom? exported-csn))

;; Test telnet port: send "help" and expect at least 250 bytes
(test-assert "telnet-help"
   (let ((response (send-recv exported-telnet "help\n.\n")))
      (>= (string-length response) 250)))

;; Test telnet port: send scheme expression and verify result
(test-assert "telnet-scheme-eval"
   (let ((response (send-recv exported-telnet "scm\n(+ 2 2)\n.\n")))
      (string-contains response "4")))

;; Test web port: send HTTP GET and expect 200 OK with at least 500 bytes
(test-assert "web-stats"
   (let ((response (send-recv exported-web "GET /stats HTTP/1.1\r\nHost: localhost\r\n\r\n")))
      (and (string-contains response "200 OK")
           (>= (string-length response) 500))))

;; Stop the cogserver
(stop-cogserver)

;; Verify ports are closed after stop
(test-assert "telnet-closed-after-stop"
   (not (can-connect-to-port? exported-telnet)))

(test-assert "web-closed-after-stop"
   (not (can-connect-to-port? exported-web)))

;; -------------------------------------------------------
;; Custom helper to start a cogserver with given name and ports
(define (start-named-cogserver name telnet-port web-port)
   (define csn (CogServerNode name))
   (cog-set-value! csn (Predicate "*-telnet-port-*") (FloatValue telnet-port))
   (cog-set-value! csn (Predicate "*-web-port-*") (FloatValue web-port))
   (cog-set-value! csn (Predicate "*-mcp-port-*") (FloatValue 0))
   (cog-set-value! csn (Predicate "*-start-*") (VoidValue))
   csn)

;; Helper to stop a cogserver
(define (stop-named-cogserver csn)
   (cog-set-value! csn (Predicate "*-stop-*") (VoidValue)))

;; ===== Single cogserver =====
(define single-telnet-port 17101)
(define single-web-port 18181)
(define single-csn #f)

(test-assert "single-start"
   (begin
      (set! single-csn (start-named-cogserver "cogserver-single" single-telnet-port single-web-port))
      (cog-atom? single-csn)))

(test-assert "single-telnet-reachable"
   (can-connect-to-port? single-telnet-port))

(test-assert "single-web-reachable"
   (can-connect-to-port? single-web-port))

(test-assert "single-stop"
   (begin
      (stop-named-cogserver single-csn)
      #t))

(test-assert "single-telnet-closed"
   (not (can-connect-to-port? single-telnet-port)))

(test-assert "single-web-closed"
   (not (can-connect-to-port? single-web-port)))

;; ===== Three cogservers simultaneously =====
(define s1-telnet 17201) (define s1-web 18281)
(define s2-telnet 17202) (define s2-web 18282)
(define s3-telnet 17203) (define s3-web 18283)
(define csn1 #f)
(define csn2 #f)
(define csn3 #f)

(test-assert "three-start-A"
   (begin
      (set! csn1 (start-named-cogserver "cogserver-A" s1-telnet s1-web))
      (cog-atom? csn1)))

(test-assert "three-start-B"
   (begin
      (set! csn2 (start-named-cogserver "cogserver-B" s2-telnet s2-web))
      (cog-atom? csn2)))

(test-assert "three-start-C"
   (begin
      (set! csn3 (start-named-cogserver "cogserver-C" s3-telnet s3-web))
      (cog-atom? csn3)))

;; Test connectivity to all six ports
(test-assert "three-A-telnet-reachable"
   (can-connect-to-port? s1-telnet))

(test-assert "three-A-web-reachable"
   (can-connect-to-port? s1-web))

(test-assert "three-B-telnet-reachable"
   (can-connect-to-port? s2-telnet))

(test-assert "three-B-web-reachable"
   (can-connect-to-port? s2-web))

(test-assert "three-C-telnet-reachable"
   (can-connect-to-port? s3-telnet))

(test-assert "three-C-web-reachable"
   (can-connect-to-port? s3-web))

;; Stop all cogservers
(test-assert "three-stop-all"
   (begin
      (stop-named-cogserver csn1)
      (stop-named-cogserver csn2)
      (stop-named-cogserver csn3)
      #t))

;; Verify all ports are closed
(test-assert "three-A-telnet-closed"
   (not (can-connect-to-port? s1-telnet)))

(test-assert "three-A-web-closed"
   (not (can-connect-to-port? s1-web)))

(test-assert "three-B-telnet-closed"
   (not (can-connect-to-port? s2-telnet)))

(test-assert "three-B-web-closed"
   (not (can-connect-to-port? s2-web)))

(test-assert "three-C-telnet-closed"
   (not (can-connect-to-port? s3-telnet)))

(test-assert "three-C-web-closed"
   (not (can-connect-to-port? s3-web)))

;; -------------------------------------------------------
;; ===== Three cogservers using (start-cogserver) =====
;; Test that three servers can run simultaneously and respond correctly.

(define tri-1-telnet 17301) (define tri-1-web 18381)
(define tri-2-telnet 17302) (define tri-2-web 18382)
(define tri-3-telnet 17303) (define tri-3-web 18383)

(define tri-csn-1 (start-cogserver #:port tri-1-telnet #:web tri-1-web #:mcp 0))
(define tri-csn-2 (start-cogserver #:port tri-2-telnet #:web tri-2-web #:mcp 0))
(define tri-csn-3 (start-cogserver #:port tri-3-telnet #:web tri-3-web #:mcp 0))

(test-assert "tri-start-1" (cog-atom? tri-csn-1))
(test-assert "tri-start-2" (cog-atom? tri-csn-2))
(test-assert "tri-start-3" (cog-atom? tri-csn-3))

;; Verify all three have unique names
(test-assert "tri-unique-names"
   (let ((n1 (cog-name tri-csn-1))
         (n2 (cog-name tri-csn-2))
         (n3 (cog-name tri-csn-3)))
      (and (not (string=? n1 n2))
           (not (string=? n2 n3))
           (not (string=? n1 n3)))))

;; Test telnet: send scheme expression to each and verify result
(test-assert "tri-1-scheme"
   (string-contains (send-recv tri-1-telnet "scm\n(+ 1 1)\n.\n") "2"))

(test-assert "tri-2-scheme"
   (string-contains (send-recv tri-2-telnet "scm\n(+ 2 2)\n.\n") "4"))

(test-assert "tri-3-scheme"
   (string-contains (send-recv tri-3-telnet "scm\n(+ 3 3)\n.\n") "6"))

;; Test HTTP: send GET /stats to each and verify 200 OK
(test-assert "tri-1-http"
   (string-contains
      (send-recv tri-1-web "GET /stats HTTP/1.1\r\nHost: localhost\r\n\r\n")
      "200 OK"))

(test-assert "tri-2-http"
   (string-contains
      (send-recv tri-2-web "GET /stats HTTP/1.1\r\nHost: localhost\r\n\r\n")
      "200 OK"))

(test-assert "tri-3-http"
   (string-contains
      (send-recv tri-3-web "GET /stats HTTP/1.1\r\nHost: localhost\r\n\r\n")
      "200 OK"))

;; Test shared AtomSpace: create atom in server 1, find it in servers 2 and 3
(test-assert "tri-shared-create"
   (string-contains
      (send-recv tri-1-telnet "scm\n(Concept \"foobar\")\n.\n")
      "(Concept \"foobar\")"))

(test-assert "tri-shared-find-2"
   (string-contains
      (send-recv tri-2-telnet "scm\n(cog-node 'Concept \"foobar\")\n.\n")
      "(Concept \"foobar\")"))

(test-assert "tri-shared-find-3"
   (string-contains
      (send-recv tri-3-telnet "scm\n(cog-node 'Concept \"foobar\")\n.\n")
      "(Concept \"foobar\")"))

;; Verify "foobar" is also visible here in the main scheme
(test-assert "tri-shared-find-home"
   (cog-atom? (cog-node 'Concept "foobar")))

;; Create atom here at home, verify all three servers can find it
(Concept "wham bam")

(test-assert "tri-home-find-1"
   (string-contains
      (send-recv tri-1-telnet "scm\n(cog-node 'Concept \"wham bam\")\n.\n")
      "(Concept \"wham bam\")"))

(test-assert "tri-home-find-2"
   (string-contains
      (send-recv tri-2-telnet "scm\n(cog-node 'Concept \"wham bam\")\n.\n")
      "(Concept \"wham bam\")"))

(test-assert "tri-home-find-3"
   (string-contains
      (send-recv tri-3-telnet "scm\n(cog-node 'Concept \"wham bam\")\n.\n")
      "(Concept \"wham bam\")"))

;; Stop all three using stop-cogserver with explicit argument
(stop-cogserver tri-csn-1)
(stop-cogserver tri-csn-2)
(stop-cogserver tri-csn-3)

;; Verify all ports are closed
(test-assert "tri-1-closed" (not (can-connect-to-port? tri-1-telnet)))
(test-assert "tri-2-closed" (not (can-connect-to-port? tri-2-telnet)))
(test-assert "tri-3-closed" (not (can-connect-to-port? tri-3-telnet)))

;; -------------------------------------------------------
(test-end tname)

(opencog-test-end)
