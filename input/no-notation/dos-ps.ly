\version "2.3.16"

\header{

    texidoc = "This should not hang lilypond --safe-mode --no-pdf
    --png run.

}

\score{
    % DOS
     c''-"\\embeddedps{ { 0 0 moveto } loop }"
}
