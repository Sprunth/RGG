include(UseLATEX)

add_latex_document(
	RGGUsersGuide.tex
	INPUTS
		LatexMacros.tex
		Introduction.tex
		Installation.tex
		ConceptualOverviewAndTerminology.tex
		BasicUIOverview.tex
		ExampleBuildingARectangularCore.tex
		ExampleBuildingAHexagonalCore.tex
		WhatMakesAGoodMesh.tex
		InterfacingWithMeshKitToolsAndCubit.tex
		HowToMesh.tex
		DisplayingMeshes.tex
		AdvancedTechniques.tex
		multind.sty
	DEFAULT_PDF
	INDEX_NAMES keyword ui
	USE_INDEX
	MANGLE_TARGET_NAMES
	IMAGE_DIRS Images
)