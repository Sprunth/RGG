include(UseLATEX)

add_latex_document(
	RGGUsersGuide.tex
	INPUTS
		Introduction.tex
		Installation.tex
		ConceptualOverviewAndTerminology.tex
		BasicUIOverview.tex
		InterfacingWithMeshKitToolsAndCubit.tex
		HowToMesh.tex
		DisplayingMeshes.tex
		ExampleBuildingAHexagonalCore.tex
		ExampleBuildingARectangularCore.tex
	DEFAULT_PDF
	MANGLE_TARGET_NAMES
	IMAGE_DIRS Images
)