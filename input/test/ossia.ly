\version "1.7.16"
\header {

texidoc = "Ossias present alternatives for a piece. They are not
really supported, but can be somewhat faked in lily."

}

\score {
\notes { c1 c1 <
	{\clef "treble" c1 c1 }
	\context Staff = stb { c1 c1 }
	>
}
}
%% new-chords-done %%
