AUTOMAKE_OPTIONS = no-dependencies

noinst_PROGRAMS = \
	testver testtypes testcpuinfo testhread testlock testerror testsem testtimer \
	loopwave testcdrom testkeys testvidinfo checkkeys testwin graywin \
	testsprite testbitmap testalpha testgamma testpalette testwm \
	threadwin testoverlay testoverlay2 testgl testdyngl testjoystick

testalpha_SOURCES = testalpha.c
testalpha_LDADD = @MATHLIB@
testgamma_SOURCES = testgamma.c
testgamma_LDADD = @MATHLIB@
testpalette_SOURCES = testpalette.c
testpalette_LDADD = @MATHLIB@
testgl_SOURCES = testgl.c
testgl_LDADD = @GL_LIBS@ @MATHLIB@
