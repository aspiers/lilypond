\version "2.1.28"

\header{
    
    texidoc = "This should not hang lilypond --safe-mode run."

}

\score{
    % DOS
    \notes c''-"\\loop\\iftrue\\repeat"
}
