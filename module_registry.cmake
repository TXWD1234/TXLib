include_guard(GLOBAL)

include("${TXLib_INSTALLATION_DIR}/module_registry_register.cmake")

set(TXLib_MODULES)

tx_module_registry_register(
	TXFoundation "TXFoundation"
)
tx_module_registry_register(
	TXMath "TXMath"
	TXFoundation
)
tx_module_registry_register(
	TXData "TXData"
	TXFoundation
)
tx_module_registry_register(
	TXUtility "TXUtility"
	TXFoundation
	TXData
)
tx_module_registry_register(
	TXGraphics "TXGraphics"
	TXFoundation
	TXMath
	TXData
	TXUtility
)
tx_module_registry_register(
	TXGrid "TXGrid"
	TXMath
	TXData
	TXUtility
)
tx_module_registry_register(
	TXResource "TXResource"
	TXMath
	TXData
	TXUtility
)
tx_module_registry_register(
	TXJson "TXJson"
	TXMath
	TXData
	TXUtility
)
