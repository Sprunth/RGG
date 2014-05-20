include(UseLATEX)

add_latex_document(
	RGGUsersGuide.tex
	INPUTS
		Introduction.tex
		MainApplicationComponents.tex
		ExampleBuildingARectangularCore.tex
		ExampleBuildingAHexagonalCore.tex
		SpecifyingMeshingControlInformation.tex
		InterfacingWithMeshKitToolsAndCubit.tex
		DisplayingMeshes.tex
	DEFAULT_PDF
	MANGLE_TARGET_NAMES
)