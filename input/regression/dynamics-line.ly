\version "1.7.16"
\header{
texidoc=" Dynamics appear below or above the staff.  If multiple
dynamics are linked with (de)crescendi, they should be on the same
line. Isolated dynamics may be forced up or down.
 "
}



\score{
\notes\relative c''{
a1^\sfz
a1-\fff\> \!c,,-\pp a'' a-\p

% We need this to test if we get two Dynamic line spanners
a

% because do_removal_processing ()
% doesn't seem to post_process elements
d-\f

a

}
\paper{
}
\midi{
\tempo 1 = 60
}
}
%% new-chords-done %%
