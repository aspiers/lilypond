;;;; standalone.scm -- implement Scheme stuff for use without LilyPond
;;;;
;;;;  source file of the GNU LilyPond music typesetter
;;;; 
;;;; (c)  1998--2003 Jan Nieuwenhuizen <janneke@gnu.org>
;;;; Han-Wen Nienhuys <hanwen@cs.uu.nl>

(define standalone (not (defined? 'ly:gulp-file)))
;;(write standalone (current-error-port))

(define (gulp-file name)
  (let* ((file (open-input-file name))
	 (text (read-delimited "" file)))
    (close file)
    text))

(define (scm-gulp-file name)
  (set! %load-path 
	(cons (string-append (getenv 'LILYPONDPREFIX) "/ly")
	      (cons (string-append (getenv 'LILYPONDPREFIX) "/ps")
		    %load-path)))
  (let ((path (%search-load-path name)))
       (if path
	   (gulp-file path)
	   (gulp-file name))))

(define (scm-number->string x)
  (let ((e (inexact->exact x)))
    (string-append (if (= e x)
		       (number->string e)
		       (number->string x))
		   " ")))

(define ly:gulp-file scm-gulp-file)
(define ly:number->string scm-number->string)

(eval-string (ly:gulp-file "lily.scm"))
