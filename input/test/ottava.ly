\version "1.7.16"


fragment = \notes {
  a'''' b c a
  \property Voice.TextSpanner \set #'type = #'dotted-line
  \property Voice.TextSpanner \set #'edge-height = #'(0 . 1.5)
  \property Voice.TextSpanner \set #'edge-text = #'("8va " . "")
  \property Staff.centralCPosition = #-13
  a#(ly:export (make-span-event 'TextSpanEvent START)) b c a #(ly:export (make-span-event 'TextSpanEvent STOP))
}

\paper { raggedright = ##t} 

\score {
  \notes\relative c \fragment
  \paper { }  
}
%% new-chords-done %%
