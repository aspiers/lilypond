\version "2.3.16"
\header {
    texidoc = "In lyrics, hyphens may be used."
}
\paper {

    raggedright= ##t
}

<<
    \context Staff  { c' (c') c'( c') }
    \context Lyrics \lyricmode { bla -- alb xxx -- yyy }
>>






