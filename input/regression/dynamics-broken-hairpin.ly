\version "1.7.16"
\header{
texidoc = "Broken crescendi should look be open on one side."
}

\score { \notes \relative c'' { 
    c1 \< \break \! c1  \> \break \! c1 
  }
  \paper {
    linewidth = 10.\cm
  }
}
  
%% new-chords-done %%
