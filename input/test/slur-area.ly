\score{
\notes\relative c''{

\property Voice.slurVerticalDirection = #1
\property Voice.stemVerticalDirection = #-1
a(a' a,)a
a(a a')a,
a(d a)a
a(a d)a

e'8(e e e e e e)e
a,8(d a a a a a)a
a(a a a a a d)a

c,(g' c e f d b)g 
c,(g' c e f d b)g 

\property Voice.noAutoBeaming = ##t
\property Voice.stemVerticalDirection = #1
a8(a a a a a a)a

}
\paper{
indent=0.0\mm;
}
}
