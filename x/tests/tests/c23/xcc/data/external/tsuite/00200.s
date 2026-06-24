	.module xcc_output

	.area _DATA
_debug:
	.ds 2
_nfailed:
	.ds 2

	.area _CONST
__str_6:
	.db 37, 115, 32, 37, 100, 32, 37, 100, 10, 0
__str_19:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_32:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_48:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_61:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_77:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_90:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_106:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_119:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_138:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_151:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_167:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_180:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_196:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_209:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_225:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_238:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_260:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_273:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_289:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_302:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_318:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_331:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_347:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_360:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_379:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_392:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_408:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_421:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_437:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_450:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_466:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_479:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 49, 41, 41, 41, 0
__str_501:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_514:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_530:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_543:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_559:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_572:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_588:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_601:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_620:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_633:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_649:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_662:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_678:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_691:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_707:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_720:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_742:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_755:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_771:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_784:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_800:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_813:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_829:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_842:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_861:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_874:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_890:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_903:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_919:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_932:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_948:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_961:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 49, 41, 41, 41, 0
__str_986:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_999:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1015:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1028:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1044:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1057:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1073:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1086:
	.db 40, 40, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1105:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1118:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1134:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1147:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1163:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1176:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1192:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1205:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 115, 104, 111, 114, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1227:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1240:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1256:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1269:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1285:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1298:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1314:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1327:
	.db 40, 40, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1346:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1359:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1375:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1388:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1404:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1417:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1433:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1446:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 105, 110, 116, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1468:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1481:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1497:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1510:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1526:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1539:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1555:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1568:
	.db 40, 40, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1587:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1600:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1616:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1629:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1645:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1658:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1674:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1687:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1709:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1722:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1738:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1751:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1767:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1780:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1796:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1809:
	.db 40, 40, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1828:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1841:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1857:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1870:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1886:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1899:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1915:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1928:
	.db 40, 40, 117, 110, 115, 105, 103, 110, 101, 100, 32, 108, 111, 110, 103, 32, 108, 111, 110, 103, 41, 40, 40, 45, 49, 41, 41, 41, 0
__str_1941:
	.db 37, 100, 32, 116, 101, 115, 116, 40, 115, 41, 32, 102, 97, 105, 108, 101, 100, 10, 0


	.area _CODE

_check:
	; prologue: check (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	; receive param s at 4(ix)
	; receive param arg1 at 6(ix)
	; receive param shift at 8(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 8(ix)
	ld	h, 9(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, (_debug)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L4:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #1
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L5:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	jp	__xcc_L2
__xcc_L2:
	ld	hl, (_nfailed)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	(_nfailed), hl
__check_end:
	; epilogue: check
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param argc at 4(ix)
	; receive param argv at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_60492
	jp	p, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	(_debug), hl
__xcc_L7:
__xcc_L10:
__xcc_L13:
__xcc_L16:
	ld	hl, #__str_19
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L23
	jp	__xcc_L24
__xcc_L24:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	jp	__xcc_L25
__xcc_L23:
	ld	hl, #1
	ld	-18(ix), l
	ld	-17(ix), h
__xcc_L25:
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L20
	jp	__xcc_L21
__xcc_L20:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	jp	__xcc_L22
__xcc_L21:
	ld	hl, #1
	ld	-22(ix), l
	ld	-21(ix), h
__xcc_L22:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	.globl __mul16
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	b, l
	pop	hl
__shift_540:
	ld	a, b
	or	a, a
	jp	z, __sdone_3426
	add	hl, hl
	djnz	__shift_540
__sdone_3426:
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89172
	ld	hl, #0
	jp	__cmp_e_55736
__cmp_t_89172:
	ld	hl, #1
__cmp_e_55736:
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L29
	jp	__xcc_L30
__xcc_L30:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	b, l
	pop	hl
__shift_2567:
	ld	a, b
	or	a, a
	jp	z, __sdone_6429
	add	hl, hl
	djnz	__shift_2567
__sdone_6429:
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65782
	ld	hl, #0
	jp	__cmp_e_21530
__cmp_t_65782:
	ld	hl, #1
__cmp_e_21530:
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22862
	ld	hl, #0
	jp	__cmp_e_65123
__cmp_t_22862:
	ld	hl, #1
__cmp_e_65123:
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	jp	__xcc_L31
__xcc_L29:
	ld	hl, #1
	ld	-48(ix), l
	ld	-47(ix), h
__xcc_L31:
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L26
	jp	__xcc_L27
__xcc_L26:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	jp	__xcc_L28
__xcc_L27:
	ld	hl, #1
	ld	-52(ix), l
	ld	-51(ix), h
__xcc_L28:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	.globl __mul16
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	l, -52(ix)
	ld	h, -51(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	l, -56(ix)
	ld	h, -55(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_32
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74067
	ld	hl, #0
	jp	__cmp_e_3135
__cmp_t_74067:
	ld	hl, #1
__cmp_e_3135:
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13929
	ld	hl, #0
	jp	__cmp_e_79802
__cmp_t_13929:
	ld	hl, #1
__cmp_e_79802:
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L36
	jp	__xcc_L37
__xcc_L37:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_34022
	ld	hl, #0
	jp	__cmp_e_23058
__cmp_t_34022:
	ld	hl, #1
__cmp_e_23058:
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33069
	ld	hl, #0
	jp	__cmp_e_98167
__cmp_t_33069:
	ld	hl, #1
__cmp_e_98167:
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	jp	__xcc_L38
__xcc_L36:
	ld	hl, #1
	ld	-72(ix), l
	ld	-71(ix), h
__xcc_L38:
	ld	l, -72(ix)
	ld	h, -71(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L33
	jp	__xcc_L34
__xcc_L33:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	jp	__xcc_L35
__xcc_L34:
	ld	hl, #1
	ld	-76(ix), l
	ld	-75(ix), h
__xcc_L35:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	.globl __mul16
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	ld	l, -84(ix)
	ld	h, -83(ix)
	ld	b, l
	pop	hl
__shift_1393:
	ld	a, b
	or	a, a
	jp	z, __sdone_8456
	add	hl, hl
	djnz	__shift_1393
__sdone_8456:
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_75011
	ld	hl, #0
	jp	__cmp_e_78042
__cmp_t_75011:
	ld	hl, #1
__cmp_e_78042:
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76229
	ld	hl, #0
	jp	__cmp_e_77373
__cmp_t_76229:
	ld	hl, #1
__cmp_e_77373:
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	l, -90(ix)
	ld	h, -89(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L42
	jp	__xcc_L43
__xcc_L43:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	push	hl
	ld	l, -94(ix)
	ld	h, -93(ix)
	ld	b, l
	pop	hl
__shift_4421:
	ld	a, b
	or	a, a
	jp	z, __sdone_4919
	add	hl, hl
	djnz	__shift_4421
__sdone_4919:
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -98(ix)
	ld	h, -97(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13784
	ld	hl, #0
	jp	__cmp_e_98537
__cmp_t_13784:
	ld	hl, #1
__cmp_e_98537:
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75198
	ld	hl, #0
	jp	__cmp_e_94324
__cmp_t_75198:
	ld	hl, #1
__cmp_e_94324:
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	jp	__xcc_L44
__xcc_L42:
	ld	hl, #1
	ld	-102(ix), l
	ld	-101(ix), h
__xcc_L44:
	ld	l, -102(ix)
	ld	h, -101(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L39
	jp	__xcc_L40
__xcc_L39:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	l, -104(ix)
	ld	h, -103(ix)
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	jp	__xcc_L41
__xcc_L40:
	ld	hl, #1
	ld	-106(ix), l
	ld	-105(ix), h
__xcc_L41:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	.globl __mul16
	ld	l, -108(ix)
	ld	h, -107(ix)
	push	hl
	ld	l, -106(ix)
	ld	h, -105(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -110(ix)
	ld	h, -109(ix)
	push	hl
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L17:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L16
	jp	__xcc_L18
__xcc_L18:
__xcc_L45:
	ld	hl, #__str_48
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98315
	ld	hl, #0
	jp	__cmp_e_64370
__cmp_t_98315:
	ld	hl, #1
__cmp_e_64370:
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	l, -116(ix)
	ld	h, -115(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66413
	ld	hl, #0
	jp	__cmp_e_3526
__cmp_t_66413:
	ld	hl, #1
__cmp_e_3526:
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L52
	jp	__xcc_L53
__xcc_L53:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_76091
	ld	hl, #0
	jp	__cmp_e_68980
__cmp_t_76091:
	ld	hl, #1
__cmp_e_68980:
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	l, -124(ix)
	ld	h, -123(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_59956
	ld	hl, #0
	jp	__cmp_e_41873
__cmp_t_59956:
	ld	hl, #1
__cmp_e_41873:
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	jp	__xcc_L54
__xcc_L52:
	ld	hl, #1
	ld	-126(ix), l
	ld	-125(ix), h
__xcc_L54:
	ld	l, -126(ix)
	ld	h, -125(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L49
	jp	__xcc_L50
__xcc_L49:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	l, -128(ix)
	ld	h, -127(ix)
	dec	sp
	dec	sp
	ld	-130(ix), l
	ld	-129(ix), h
	jp	__xcc_L51
__xcc_L50:
	ld	hl, #1
	ld	-130(ix), l
	ld	-129(ix), h
__xcc_L51:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
	.globl __mul16
	ld	l, -132(ix)
	ld	h, -131(ix)
	push	hl
	ld	l, -130(ix)
	ld	h, -129(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-134(ix), l
	ld	-133(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-136(ix), l
	ld	-135(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-138(ix), l
	ld	-137(ix), h
	ld	l, -136(ix)
	ld	h, -135(ix)
	push	hl
	ld	l, -138(ix)
	ld	h, -137(ix)
	ld	b, l
	pop	hl
__shift_6862:
	ld	a, b
	or	a, a
	jp	z, __sdone_9170
	add	hl, hl
	djnz	__shift_6862
__sdone_9170:
	dec	sp
	dec	sp
	ld	-140(ix), l
	ld	-139(ix), h
	ld	l, -140(ix)
	ld	h, -139(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_6996
	ld	hl, #0
	jp	__cmp_e_97281
__cmp_t_6996:
	ld	hl, #1
__cmp_e_97281:
	dec	sp
	dec	sp
	ld	-142(ix), l
	ld	-141(ix), h
	ld	l, -142(ix)
	ld	h, -141(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2305
	ld	hl, #0
	jp	__cmp_e_20925
__cmp_t_2305:
	ld	hl, #1
__cmp_e_20925:
	dec	sp
	dec	sp
	ld	-144(ix), l
	ld	-143(ix), h
	ld	l, -144(ix)
	ld	h, -143(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L58
	jp	__xcc_L59
__xcc_L59:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-146(ix), l
	ld	-145(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-148(ix), l
	ld	-147(ix), h
	ld	l, -146(ix)
	ld	h, -145(ix)
	push	hl
	ld	l, -148(ix)
	ld	h, -147(ix)
	ld	b, l
	pop	hl
__shift_7084:
	ld	a, b
	or	a, a
	jp	z, __sdone_6327
	add	hl, hl
	djnz	__shift_7084
__sdone_6327:
	dec	sp
	dec	sp
	ld	-150(ix), l
	ld	-149(ix), h
	ld	l, -150(ix)
	ld	h, -149(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-152(ix), l
	ld	-151(ix), h
	ld	l, -152(ix)
	ld	h, -151(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60336
	ld	hl, #0
	jp	__cmp_e_26505
__cmp_t_60336:
	ld	hl, #1
__cmp_e_26505:
	dec	sp
	dec	sp
	ld	-154(ix), l
	ld	-153(ix), h
	ld	l, -154(ix)
	ld	h, -153(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50846
	ld	hl, #0
	jp	__cmp_e_21729
__cmp_t_50846:
	ld	hl, #1
__cmp_e_21729:
	dec	sp
	dec	sp
	ld	-156(ix), l
	ld	-155(ix), h
	jp	__xcc_L60
__xcc_L58:
	ld	hl, #1
	ld	-156(ix), l
	ld	-155(ix), h
__xcc_L60:
	ld	l, -156(ix)
	ld	h, -155(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L55
	jp	__xcc_L56
__xcc_L55:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-158(ix), l
	ld	-157(ix), h
	ld	l, -158(ix)
	ld	h, -157(ix)
	dec	sp
	dec	sp
	ld	-160(ix), l
	ld	-159(ix), h
	jp	__xcc_L57
__xcc_L56:
	ld	hl, #1
	ld	-160(ix), l
	ld	-159(ix), h
__xcc_L57:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-162(ix), l
	ld	-161(ix), h
	.globl __mul16
	ld	l, -162(ix)
	ld	h, -161(ix)
	push	hl
	ld	l, -160(ix)
	ld	h, -159(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-164(ix), l
	ld	-163(ix), h
	ld	l, -164(ix)
	ld	h, -163(ix)
	push	hl
	ld	l, -134(ix)
	ld	h, -133(ix)
	push	hl
	ld	l, -112(ix)
	ld	h, -111(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_61
	dec	sp
	dec	sp
	ld	-166(ix), l
	ld	-165(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-168(ix), l
	ld	-167(ix), h
	ld	l, -168(ix)
	ld	h, -167(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_61313
	ld	hl, #0
	jp	__cmp_e_25857
__cmp_t_61313:
	ld	hl, #1
__cmp_e_25857:
	dec	sp
	dec	sp
	ld	-170(ix), l
	ld	-169(ix), h
	ld	l, -170(ix)
	ld	h, -169(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16124
	ld	hl, #0
	jp	__cmp_e_53895
__cmp_t_16124:
	ld	hl, #1
__cmp_e_53895:
	dec	sp
	dec	sp
	ld	-172(ix), l
	ld	-171(ix), h
	ld	l, -172(ix)
	ld	h, -171(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L65
	jp	__xcc_L66
__xcc_L66:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-174(ix), l
	ld	-173(ix), h
	ld	l, -174(ix)
	ld	h, -173(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-176(ix), l
	ld	-175(ix), h
	ld	l, -176(ix)
	ld	h, -175(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_19582
	ld	hl, #0
	jp	__cmp_e_545
__cmp_t_19582:
	ld	hl, #1
__cmp_e_545:
	dec	sp
	dec	sp
	ld	-178(ix), l
	ld	-177(ix), h
	ld	l, -178(ix)
	ld	h, -177(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98814
	ld	hl, #0
	jp	__cmp_e_33367
__cmp_t_98814:
	ld	hl, #1
__cmp_e_33367:
	dec	sp
	dec	sp
	ld	-180(ix), l
	ld	-179(ix), h
	jp	__xcc_L67
__xcc_L65:
	ld	hl, #1
	ld	-180(ix), l
	ld	-179(ix), h
__xcc_L67:
	ld	l, -180(ix)
	ld	h, -179(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L62
	jp	__xcc_L63
__xcc_L62:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-182(ix), l
	ld	-181(ix), h
	ld	l, -182(ix)
	ld	h, -181(ix)
	dec	sp
	dec	sp
	ld	-184(ix), l
	ld	-183(ix), h
	jp	__xcc_L64
__xcc_L63:
	ld	hl, #1
	ld	-184(ix), l
	ld	-183(ix), h
__xcc_L64:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-186(ix), l
	ld	-185(ix), h
	.globl __mul16
	ld	l, -186(ix)
	ld	h, -185(ix)
	push	hl
	ld	l, -184(ix)
	ld	h, -183(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-188(ix), l
	ld	-187(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-190(ix), l
	ld	-189(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-192(ix), l
	ld	-191(ix), h
	ld	l, -190(ix)
	ld	h, -189(ix)
	push	hl
	ld	l, -192(ix)
	ld	h, -191(ix)
	ld	b, l
	pop	hl
__shift_5434:
	ld	a, b
	or	a, a
	jp	z, __sdone_364
	add	hl, hl
	djnz	__shift_5434
__sdone_364:
	dec	sp
	dec	sp
	ld	-194(ix), l
	ld	-193(ix), h
	ld	l, -194(ix)
	ld	h, -193(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_44043
	ld	hl, #0
	jp	__cmp_e_13750
__cmp_t_44043:
	ld	hl, #1
__cmp_e_13750:
	dec	sp
	dec	sp
	ld	-196(ix), l
	ld	-195(ix), h
	ld	l, -196(ix)
	ld	h, -195(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71087
	ld	hl, #0
	jp	__cmp_e_26808
__cmp_t_71087:
	ld	hl, #1
__cmp_e_26808:
	dec	sp
	dec	sp
	ld	-198(ix), l
	ld	-197(ix), h
	ld	l, -198(ix)
	ld	h, -197(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L71
	jp	__xcc_L72
__xcc_L72:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-200(ix), l
	ld	-199(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-202(ix), l
	ld	-201(ix), h
	ld	l, -200(ix)
	ld	h, -199(ix)
	push	hl
	ld	l, -202(ix)
	ld	h, -201(ix)
	ld	b, l
	pop	hl
__shift_7276:
	ld	a, b
	or	a, a
	jp	z, __sdone_7178
	add	hl, hl
	djnz	__shift_7276
__sdone_7178:
	dec	sp
	dec	sp
	ld	-204(ix), l
	ld	-203(ix), h
	ld	l, -204(ix)
	ld	h, -203(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-206(ix), l
	ld	-205(ix), h
	ld	l, -206(ix)
	ld	h, -205(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_95788
	ld	hl, #0
	jp	__cmp_e_93584
__cmp_t_95788:
	ld	hl, #1
__cmp_e_93584:
	dec	sp
	dec	sp
	ld	-208(ix), l
	ld	-207(ix), h
	ld	l, -208(ix)
	ld	h, -207(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_5403
	ld	hl, #0
	jp	__cmp_e_2651
__cmp_t_5403:
	ld	hl, #1
__cmp_e_2651:
	dec	sp
	dec	sp
	ld	-210(ix), l
	ld	-209(ix), h
	jp	__xcc_L73
__xcc_L71:
	ld	hl, #1
	ld	-210(ix), l
	ld	-209(ix), h
__xcc_L73:
	ld	l, -210(ix)
	ld	h, -209(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L68
	jp	__xcc_L69
__xcc_L68:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-212(ix), l
	ld	-211(ix), h
	ld	l, -212(ix)
	ld	h, -211(ix)
	dec	sp
	dec	sp
	ld	-214(ix), l
	ld	-213(ix), h
	jp	__xcc_L70
__xcc_L69:
	ld	hl, #1
	ld	-214(ix), l
	ld	-213(ix), h
__xcc_L70:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-216(ix), l
	ld	-215(ix), h
	.globl __mul16
	ld	l, -216(ix)
	ld	h, -215(ix)
	push	hl
	ld	l, -214(ix)
	ld	h, -213(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-218(ix), l
	ld	-217(ix), h
	ld	l, -218(ix)
	ld	h, -217(ix)
	push	hl
	ld	l, -188(ix)
	ld	h, -187(ix)
	push	hl
	ld	l, -166(ix)
	ld	h, -165(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L46:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L45
	jp	__xcc_L47
__xcc_L47:
__xcc_L74:
	ld	hl, #__str_77
	dec	sp
	dec	sp
	ld	-220(ix), l
	ld	-219(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-222(ix), l
	ld	-221(ix), h
	ld	l, -222(ix)
	ld	h, -221(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92754
	ld	hl, #0
	jp	__cmp_e_12399
__cmp_t_92754:
	ld	hl, #1
__cmp_e_12399:
	dec	sp
	dec	sp
	ld	-224(ix), l
	ld	-223(ix), h
	ld	l, -224(ix)
	ld	h, -223(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_99932
	ld	hl, #0
	jp	__cmp_e_95060
__cmp_t_99932:
	ld	hl, #1
__cmp_e_95060:
	dec	sp
	dec	sp
	ld	-226(ix), l
	ld	-225(ix), h
	ld	l, -226(ix)
	ld	h, -225(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L81
	jp	__xcc_L82
__xcc_L82:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-228(ix), l
	ld	-227(ix), h
	ld	l, -228(ix)
	ld	h, -227(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-230(ix), l
	ld	-229(ix), h
	ld	l, -230(ix)
	ld	h, -229(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49676
	ld	hl, #0
	jp	__cmp_e_93368
__cmp_t_49676:
	ld	hl, #1
__cmp_e_93368:
	dec	sp
	dec	sp
	ld	-232(ix), l
	ld	-231(ix), h
	ld	l, -232(ix)
	ld	h, -231(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47739
	ld	hl, #0
	jp	__cmp_e_10012
__cmp_t_47739:
	ld	hl, #1
__cmp_e_10012:
	dec	sp
	dec	sp
	ld	-234(ix), l
	ld	-233(ix), h
	jp	__xcc_L83
__xcc_L81:
	ld	hl, #1
	ld	-234(ix), l
	ld	-233(ix), h
__xcc_L83:
	ld	l, -234(ix)
	ld	h, -233(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L78
	jp	__xcc_L79
__xcc_L78:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-236(ix), l
	ld	-235(ix), h
	ld	l, -236(ix)
	ld	h, -235(ix)
	dec	sp
	dec	sp
	ld	-238(ix), l
	ld	-237(ix), h
	jp	__xcc_L80
__xcc_L79:
	ld	hl, #1
	ld	-238(ix), l
	ld	-237(ix), h
__xcc_L80:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-240(ix), l
	ld	-239(ix), h
	.globl __mul16
	ld	l, -240(ix)
	ld	h, -239(ix)
	push	hl
	ld	l, -238(ix)
	ld	h, -237(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-242(ix), l
	ld	-241(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-244(ix), l
	ld	-243(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-248(ix), l
	ld	-247(ix), h
	ld	l, -244(ix)
	ld	h, -243(ix)
	push	hl
	ld	l, -248(ix)
	ld	h, -247(ix)
	ld	b, l
	pop	hl
__shift_6226:
	ld	a, b
	or	a, a
	jp	z, __sdone_8586
	add	hl, hl
	djnz	__shift_6226
__sdone_8586:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-252(ix), l
	ld	-251(ix), h
	ld	l, -252(ix)
	ld	h, -251(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_48094
	ld	hl, #0
	jp	__cmp_e_97539
__cmp_t_48094:
	ld	hl, #1
__cmp_e_97539:
	dec	sp
	dec	sp
	ld	-254(ix), l
	ld	-253(ix), h
	ld	l, -254(ix)
	ld	h, -253(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_40795
	ld	hl, #0
	jp	__cmp_e_80570
__cmp_t_40795:
	ld	hl, #1
__cmp_e_80570:
	dec	sp
	dec	sp
	ld	-256(ix), l
	ld	-255(ix), h
	ld	l, -256(ix)
	ld	h, -255(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L87
	jp	__xcc_L88
__xcc_L88:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-258(ix), l
	ld	-257(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-262(ix), l
	ld	-261(ix), h
	ld	l, -258(ix)
	ld	h, -257(ix)
	push	hl
	ld	l, -262(ix)
	ld	h, -261(ix)
	ld	b, l
	pop	hl
__shift_1434:
	ld	a, b
	or	a, a
	jp	z, __sdone_378
	add	hl, hl
	djnz	__shift_1434
__sdone_378:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-266(ix), l
	ld	-265(ix), h
	ld	l, -266(ix)
	ld	h, -265(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-268(ix), l
	ld	-267(ix), h
	ld	l, -268(ix)
	ld	h, -267(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97467
	ld	hl, #0
	jp	__cmp_e_66601
__cmp_t_97467:
	ld	hl, #1
__cmp_e_66601:
	dec	sp
	dec	sp
	ld	-270(ix), l
	ld	-269(ix), h
	ld	l, -270(ix)
	ld	h, -269(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10097
	ld	hl, #0
	jp	__cmp_e_12902
__cmp_t_10097:
	ld	hl, #1
__cmp_e_12902:
	dec	sp
	dec	sp
	ld	-272(ix), l
	ld	-271(ix), h
	jp	__xcc_L89
__xcc_L87:
	ld	hl, #1
	ld	-272(ix), l
	ld	-271(ix), h
__xcc_L89:
	ld	l, -272(ix)
	ld	h, -271(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L84
	jp	__xcc_L85
__xcc_L84:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-274(ix), l
	ld	-273(ix), h
	ld	l, -274(ix)
	ld	h, -273(ix)
	dec	sp
	dec	sp
	ld	-276(ix), l
	ld	-275(ix), h
	jp	__xcc_L86
__xcc_L85:
	ld	hl, #1
	ld	-276(ix), l
	ld	-275(ix), h
__xcc_L86:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-278(ix), l
	ld	-277(ix), h
	.globl __mul16
	ld	l, -278(ix)
	ld	h, -277(ix)
	push	hl
	ld	l, -276(ix)
	ld	h, -275(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-280(ix), l
	ld	-279(ix), h
	ld	l, -280(ix)
	ld	h, -279(ix)
	push	hl
	ld	l, -242(ix)
	ld	h, -241(ix)
	push	hl
	ld	l, -220(ix)
	ld	h, -219(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_90
	dec	sp
	dec	sp
	ld	-282(ix), l
	ld	-281(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-284(ix), l
	ld	-283(ix), h
	ld	l, -284(ix)
	ld	h, -283(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73317
	ld	hl, #0
	jp	__cmp_e_70492
__cmp_t_73317:
	ld	hl, #1
__cmp_e_70492:
	dec	sp
	dec	sp
	ld	-286(ix), l
	ld	-285(ix), h
	ld	l, -286(ix)
	ld	h, -285(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26652
	ld	hl, #0
	jp	__cmp_e_60756
__cmp_t_26652:
	ld	hl, #1
__cmp_e_60756:
	dec	sp
	dec	sp
	ld	-288(ix), l
	ld	-287(ix), h
	ld	l, -288(ix)
	ld	h, -287(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L94
	jp	__xcc_L95
__xcc_L95:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-290(ix), l
	ld	-289(ix), h
	ld	l, -290(ix)
	ld	h, -289(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-292(ix), l
	ld	-291(ix), h
	ld	l, -292(ix)
	ld	h, -291(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97301
	ld	hl, #0
	jp	__cmp_e_60280
__cmp_t_97301:
	ld	hl, #1
__cmp_e_60280:
	dec	sp
	dec	sp
	ld	-294(ix), l
	ld	-293(ix), h
	ld	l, -294(ix)
	ld	h, -293(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_24286
	ld	hl, #0
	jp	__cmp_e_9441
__cmp_t_24286:
	ld	hl, #1
__cmp_e_9441:
	dec	sp
	dec	sp
	ld	-296(ix), l
	ld	-295(ix), h
	jp	__xcc_L96
__xcc_L94:
	ld	hl, #1
	ld	-296(ix), l
	ld	-295(ix), h
__xcc_L96:
	ld	l, -296(ix)
	ld	h, -295(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L91
	jp	__xcc_L92
__xcc_L91:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-298(ix), l
	ld	-297(ix), h
	ld	l, -298(ix)
	ld	h, -297(ix)
	dec	sp
	dec	sp
	ld	-300(ix), l
	ld	-299(ix), h
	jp	__xcc_L93
__xcc_L92:
	ld	hl, #1
	ld	-300(ix), l
	ld	-299(ix), h
__xcc_L93:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-302(ix), l
	ld	-301(ix), h
	.globl __mul16
	ld	l, -302(ix)
	ld	h, -301(ix)
	push	hl
	ld	l, -300(ix)
	ld	h, -299(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-304(ix), l
	ld	-303(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-306(ix), l
	ld	-305(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-310(ix), l
	ld	-309(ix), h
	ld	l, -306(ix)
	ld	h, -305(ix)
	push	hl
	ld	l, -310(ix)
	ld	h, -309(ix)
	ld	b, l
	pop	hl
__shift_3865:
	ld	a, b
	or	a, a
	jp	z, __sdone_9689
	add	hl, hl
	djnz	__shift_3865
__sdone_9689:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-314(ix), l
	ld	-313(ix), h
	ld	l, -314(ix)
	ld	h, -313(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28444
	ld	hl, #0
	jp	__cmp_e_46619
__cmp_t_28444:
	ld	hl, #1
__cmp_e_46619:
	dec	sp
	dec	sp
	ld	-316(ix), l
	ld	-315(ix), h
	ld	l, -316(ix)
	ld	h, -315(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58440
	ld	hl, #0
	jp	__cmp_e_44729
__cmp_t_58440:
	ld	hl, #1
__cmp_e_44729:
	dec	sp
	dec	sp
	ld	-318(ix), l
	ld	-317(ix), h
	ld	l, -318(ix)
	ld	h, -317(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L100
	jp	__xcc_L101
__xcc_L101:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-320(ix), l
	ld	-319(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-324(ix), l
	ld	-323(ix), h
	ld	l, -320(ix)
	ld	h, -319(ix)
	push	hl
	ld	l, -324(ix)
	ld	h, -323(ix)
	ld	b, l
	pop	hl
__shift_8031:
	ld	a, b
	or	a, a
	jp	z, __sdone_8117
	add	hl, hl
	djnz	__shift_8031
__sdone_8117:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-328(ix), l
	ld	-327(ix), h
	ld	l, -328(ix)
	ld	h, -327(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-330(ix), l
	ld	-329(ix), h
	ld	l, -330(ix)
	ld	h, -329(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_38097
	ld	hl, #0
	jp	__cmp_e_5771
__cmp_t_38097:
	ld	hl, #1
__cmp_e_5771:
	dec	sp
	dec	sp
	ld	-332(ix), l
	ld	-331(ix), h
	ld	l, -332(ix)
	ld	h, -331(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34481
	ld	hl, #0
	jp	__cmp_e_90675
__cmp_t_34481:
	ld	hl, #1
__cmp_e_90675:
	dec	sp
	dec	sp
	ld	-334(ix), l
	ld	-333(ix), h
	jp	__xcc_L102
__xcc_L100:
	ld	hl, #1
	ld	-334(ix), l
	ld	-333(ix), h
__xcc_L102:
	ld	l, -334(ix)
	ld	h, -333(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L97
	jp	__xcc_L98
__xcc_L97:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-336(ix), l
	ld	-335(ix), h
	ld	l, -336(ix)
	ld	h, -335(ix)
	dec	sp
	dec	sp
	ld	-338(ix), l
	ld	-337(ix), h
	jp	__xcc_L99
__xcc_L98:
	ld	hl, #1
	ld	-338(ix), l
	ld	-337(ix), h
__xcc_L99:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-340(ix), l
	ld	-339(ix), h
	.globl __mul16
	ld	l, -340(ix)
	ld	h, -339(ix)
	push	hl
	ld	l, -338(ix)
	ld	h, -337(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-342(ix), l
	ld	-341(ix), h
	ld	l, -342(ix)
	ld	h, -341(ix)
	push	hl
	ld	l, -304(ix)
	ld	h, -303(ix)
	push	hl
	ld	l, -282(ix)
	ld	h, -281(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L75:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L74
	jp	__xcc_L76
__xcc_L76:
__xcc_L103:
	ld	hl, #__str_106
	dec	sp
	dec	sp
	ld	-344(ix), l
	ld	-343(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-346(ix), l
	ld	-345(ix), h
	ld	l, -346(ix)
	ld	h, -345(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20709
	ld	hl, #0
	jp	__cmp_e_98927
__cmp_t_20709:
	ld	hl, #1
__cmp_e_98927:
	dec	sp
	dec	sp
	ld	-348(ix), l
	ld	-347(ix), h
	ld	l, -348(ix)
	ld	h, -347(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4567
	ld	hl, #0
	jp	__cmp_e_77856
__cmp_t_4567:
	ld	hl, #1
__cmp_e_77856:
	dec	sp
	dec	sp
	ld	-350(ix), l
	ld	-349(ix), h
	ld	l, -350(ix)
	ld	h, -349(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L110
	jp	__xcc_L111
__xcc_L111:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-352(ix), l
	ld	-351(ix), h
	ld	l, -352(ix)
	ld	h, -351(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-354(ix), l
	ld	-353(ix), h
	ld	l, -354(ix)
	ld	h, -353(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79497
	ld	hl, #0
	jp	__cmp_e_72353
__cmp_t_79497:
	ld	hl, #1
__cmp_e_72353:
	dec	sp
	dec	sp
	ld	-356(ix), l
	ld	-355(ix), h
	ld	l, -356(ix)
	ld	h, -355(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_54586
	ld	hl, #0
	jp	__cmp_e_76965
__cmp_t_54586:
	ld	hl, #1
__cmp_e_76965:
	dec	sp
	dec	sp
	ld	-358(ix), l
	ld	-357(ix), h
	jp	__xcc_L112
__xcc_L110:
	ld	hl, #1
	ld	-358(ix), l
	ld	-357(ix), h
__xcc_L112:
	ld	l, -358(ix)
	ld	h, -357(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L107
	jp	__xcc_L108
__xcc_L107:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-360(ix), l
	ld	-359(ix), h
	ld	l, -360(ix)
	ld	h, -359(ix)
	dec	sp
	dec	sp
	ld	-362(ix), l
	ld	-361(ix), h
	jp	__xcc_L109
__xcc_L108:
	ld	hl, #1
	ld	-362(ix), l
	ld	-361(ix), h
__xcc_L109:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-364(ix), l
	ld	-363(ix), h
	.globl __mul16
	ld	l, -364(ix)
	ld	h, -363(ix)
	push	hl
	ld	l, -362(ix)
	ld	h, -361(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-366(ix), l
	ld	-365(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-368(ix), l
	ld	-367(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-376(ix), l
	ld	-375(ix), h
	ld	l, -368(ix)
	ld	h, -367(ix)
	push	hl
	ld	l, -376(ix)
	ld	h, -375(ix)
	ld	b, l
	pop	hl
__shift_5306:
	ld	a, b
	or	a, a
	jp	z, __sdone_4683
	add	hl, hl
	djnz	__shift_5306
__sdone_4683:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-384(ix), l
	ld	-383(ix), h
	ld	l, -384(ix)
	ld	h, -383(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_6219
	ld	hl, #0
	jp	__cmp_e_28624
__cmp_t_6219:
	ld	hl, #1
__cmp_e_28624:
	dec	sp
	dec	sp
	ld	-386(ix), l
	ld	-385(ix), h
	ld	l, -386(ix)
	ld	h, -385(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51528
	ld	hl, #0
	jp	__cmp_e_32871
__cmp_t_51528:
	ld	hl, #1
__cmp_e_32871:
	dec	sp
	dec	sp
	ld	-388(ix), l
	ld	-387(ix), h
	ld	l, -388(ix)
	ld	h, -387(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L116
	jp	__xcc_L117
__xcc_L117:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-390(ix), l
	ld	-389(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-398(ix), l
	ld	-397(ix), h
	ld	l, -390(ix)
	ld	h, -389(ix)
	push	hl
	ld	l, -398(ix)
	ld	h, -397(ix)
	ld	b, l
	pop	hl
__shift_5732:
	ld	a, b
	or	a, a
	jp	z, __sdone_8829
	add	hl, hl
	djnz	__shift_5732
__sdone_8829:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-406(ix), l
	ld	-405(ix), h
	ld	l, -406(ix)
	ld	h, -405(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-408(ix), l
	ld	-407(ix), h
	ld	l, -408(ix)
	ld	h, -407(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_9503
	ld	hl, #0
	jp	__cmp_e_30019
__cmp_t_9503:
	ld	hl, #1
__cmp_e_30019:
	dec	sp
	dec	sp
	ld	-410(ix), l
	ld	-409(ix), h
	ld	l, -410(ix)
	ld	h, -409(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58270
	ld	hl, #0
	jp	__cmp_e_63368
__cmp_t_58270:
	ld	hl, #1
__cmp_e_63368:
	dec	sp
	dec	sp
	ld	-412(ix), l
	ld	-411(ix), h
	jp	__xcc_L118
__xcc_L116:
	ld	hl, #1
	ld	-412(ix), l
	ld	-411(ix), h
__xcc_L118:
	ld	l, -412(ix)
	ld	h, -411(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L113
	jp	__xcc_L114
__xcc_L113:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-414(ix), l
	ld	-413(ix), h
	ld	l, -414(ix)
	ld	h, -413(ix)
	dec	sp
	dec	sp
	ld	-416(ix), l
	ld	-415(ix), h
	jp	__xcc_L115
__xcc_L114:
	ld	hl, #1
	ld	-416(ix), l
	ld	-415(ix), h
__xcc_L115:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-418(ix), l
	ld	-417(ix), h
	.globl __mul16
	ld	l, -418(ix)
	ld	h, -417(ix)
	push	hl
	ld	l, -416(ix)
	ld	h, -415(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-420(ix), l
	ld	-419(ix), h
	ld	l, -420(ix)
	ld	h, -419(ix)
	push	hl
	ld	l, -366(ix)
	ld	h, -365(ix)
	push	hl
	ld	l, -344(ix)
	ld	h, -343(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_119
	dec	sp
	dec	sp
	ld	-422(ix), l
	ld	-421(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-424(ix), l
	ld	-423(ix), h
	ld	l, -424(ix)
	ld	h, -423(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59708
	ld	hl, #0
	jp	__cmp_e_86715
__cmp_t_59708:
	ld	hl, #1
__cmp_e_86715:
	dec	sp
	dec	sp
	ld	-426(ix), l
	ld	-425(ix), h
	ld	l, -426(ix)
	ld	h, -425(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26340
	ld	hl, #0
	jp	__cmp_e_18149
__cmp_t_26340:
	ld	hl, #1
__cmp_e_18149:
	dec	sp
	dec	sp
	ld	-428(ix), l
	ld	-427(ix), h
	ld	l, -428(ix)
	ld	h, -427(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L123
	jp	__xcc_L124
__xcc_L124:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-430(ix), l
	ld	-429(ix), h
	ld	l, -430(ix)
	ld	h, -429(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-432(ix), l
	ld	-431(ix), h
	ld	l, -432(ix)
	ld	h, -431(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47796
	ld	hl, #0
	jp	__cmp_e_723
__cmp_t_47796:
	ld	hl, #1
__cmp_e_723:
	dec	sp
	dec	sp
	ld	-434(ix), l
	ld	-433(ix), h
	ld	l, -434(ix)
	ld	h, -433(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_42618
	ld	hl, #0
	jp	__cmp_e_2245
__cmp_t_42618:
	ld	hl, #1
__cmp_e_2245:
	dec	sp
	dec	sp
	ld	-436(ix), l
	ld	-435(ix), h
	jp	__xcc_L125
__xcc_L123:
	ld	hl, #1
	ld	-436(ix), l
	ld	-435(ix), h
__xcc_L125:
	ld	l, -436(ix)
	ld	h, -435(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L120
	jp	__xcc_L121
__xcc_L120:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-438(ix), l
	ld	-437(ix), h
	ld	l, -438(ix)
	ld	h, -437(ix)
	dec	sp
	dec	sp
	ld	-440(ix), l
	ld	-439(ix), h
	jp	__xcc_L122
__xcc_L121:
	ld	hl, #1
	ld	-440(ix), l
	ld	-439(ix), h
__xcc_L122:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-442(ix), l
	ld	-441(ix), h
	.globl __mul16
	ld	l, -442(ix)
	ld	h, -441(ix)
	push	hl
	ld	l, -440(ix)
	ld	h, -439(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-444(ix), l
	ld	-443(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-446(ix), l
	ld	-445(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-454(ix), l
	ld	-453(ix), h
	ld	l, -446(ix)
	ld	h, -445(ix)
	push	hl
	ld	l, -454(ix)
	ld	h, -453(ix)
	ld	b, l
	pop	hl
__shift_2846:
	ld	a, b
	or	a, a
	jp	z, __sdone_3451
	add	hl, hl
	djnz	__shift_2846
__sdone_3451:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-462(ix), l
	ld	-461(ix), h
	ld	l, -462(ix)
	ld	h, -461(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92921
	ld	hl, #0
	jp	__cmp_e_43555
__cmp_t_92921:
	ld	hl, #1
__cmp_e_43555:
	dec	sp
	dec	sp
	ld	-464(ix), l
	ld	-463(ix), h
	ld	l, -464(ix)
	ld	h, -463(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92379
	ld	hl, #0
	jp	__cmp_e_97488
__cmp_t_92379:
	ld	hl, #1
__cmp_e_97488:
	dec	sp
	dec	sp
	ld	-466(ix), l
	ld	-465(ix), h
	ld	l, -466(ix)
	ld	h, -465(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L129
	jp	__xcc_L130
__xcc_L130:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-468(ix), l
	ld	-467(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-476(ix), l
	ld	-475(ix), h
	ld	l, -468(ix)
	ld	h, -467(ix)
	push	hl
	ld	l, -476(ix)
	ld	h, -475(ix)
	ld	b, l
	pop	hl
__shift_7764:
	ld	a, b
	or	a, a
	jp	z, __sdone_8228
	add	hl, hl
	djnz	__shift_7764
__sdone_8228:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-484(ix), l
	ld	-483(ix), h
	ld	l, -484(ix)
	ld	h, -483(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-486(ix), l
	ld	-485(ix), h
	ld	l, -486(ix)
	ld	h, -485(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_69841
	ld	hl, #0
	jp	__cmp_e_92350
__cmp_t_69841:
	ld	hl, #1
__cmp_e_92350:
	dec	sp
	dec	sp
	ld	-488(ix), l
	ld	-487(ix), h
	ld	l, -488(ix)
	ld	h, -487(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65193
	ld	hl, #0
	jp	__cmp_e_41500
__cmp_t_65193:
	ld	hl, #1
__cmp_e_41500:
	dec	sp
	dec	sp
	ld	-490(ix), l
	ld	-489(ix), h
	jp	__xcc_L131
__xcc_L129:
	ld	hl, #1
	ld	-490(ix), l
	ld	-489(ix), h
__xcc_L131:
	ld	l, -490(ix)
	ld	h, -489(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L126
	jp	__xcc_L127
__xcc_L126:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-492(ix), l
	ld	-491(ix), h
	ld	l, -492(ix)
	ld	h, -491(ix)
	dec	sp
	dec	sp
	ld	-494(ix), l
	ld	-493(ix), h
	jp	__xcc_L128
__xcc_L127:
	ld	hl, #1
	ld	-494(ix), l
	ld	-493(ix), h
__xcc_L128:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-496(ix), l
	ld	-495(ix), h
	.globl __mul16
	ld	l, -496(ix)
	ld	h, -495(ix)
	push	hl
	ld	l, -494(ix)
	ld	h, -493(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-498(ix), l
	ld	-497(ix), h
	ld	l, -498(ix)
	ld	h, -497(ix)
	push	hl
	ld	l, -444(ix)
	ld	h, -443(ix)
	push	hl
	ld	l, -422(ix)
	ld	h, -421(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L104:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L103
	jp	__xcc_L105
__xcc_L105:
__xcc_L14:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	jp	__xcc_L15
__xcc_L15:
__xcc_L132:
__xcc_L135:
	ld	hl, #__str_138
	dec	sp
	dec	sp
	ld	-500(ix), l
	ld	-499(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-502(ix), l
	ld	-501(ix), h
	ld	l, -502(ix)
	ld	h, -501(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_57034
	ld	hl, #0
	jp	__cmp_e_87764
__cmp_t_57034:
	ld	hl, #1
__cmp_e_87764:
	dec	sp
	dec	sp
	ld	-504(ix), l
	ld	-503(ix), h
	ld	l, -504(ix)
	ld	h, -503(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70124
	ld	hl, #0
	jp	__cmp_e_24914
__cmp_t_70124:
	ld	hl, #1
__cmp_e_24914:
	dec	sp
	dec	sp
	ld	-506(ix), l
	ld	-505(ix), h
	ld	l, -506(ix)
	ld	h, -505(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L142
	jp	__xcc_L143
__xcc_L143:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-508(ix), l
	ld	-507(ix), h
	ld	l, -508(ix)
	ld	h, -507(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-510(ix), l
	ld	-509(ix), h
	ld	l, -510(ix)
	ld	h, -509(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_36987
	ld	hl, #0
	jp	__cmp_e_75856
__cmp_t_36987:
	ld	hl, #1
__cmp_e_75856:
	dec	sp
	dec	sp
	ld	-512(ix), l
	ld	-511(ix), h
	ld	l, -512(ix)
	ld	h, -511(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73743
	ld	hl, #0
	jp	__cmp_e_46491
__cmp_t_73743:
	ld	hl, #1
__cmp_e_46491:
	dec	sp
	dec	sp
	ld	-514(ix), l
	ld	-513(ix), h
	jp	__xcc_L144
__xcc_L142:
	ld	hl, #1
	ld	-514(ix), l
	ld	-513(ix), h
__xcc_L144:
	ld	l, -514(ix)
	ld	h, -513(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L139
	jp	__xcc_L140
__xcc_L139:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-516(ix), l
	ld	-515(ix), h
	ld	l, -516(ix)
	ld	h, -515(ix)
	dec	sp
	dec	sp
	ld	-518(ix), l
	ld	-517(ix), h
	jp	__xcc_L141
__xcc_L140:
	ld	hl, #1
	ld	-518(ix), l
	ld	-517(ix), h
__xcc_L141:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-520(ix), l
	ld	-519(ix), h
	.globl __mul16
	ld	l, -520(ix)
	ld	h, -519(ix)
	push	hl
	ld	l, -518(ix)
	ld	h, -517(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-522(ix), l
	ld	-521(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-524(ix), l
	ld	-523(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-526(ix), l
	ld	-525(ix), h
	ld	l, -524(ix)
	ld	h, -523(ix)
	push	hl
	ld	l, -526(ix)
	ld	h, -525(ix)
	ld	b, l
	pop	hl
__shift_2227:
	ld	a, b
	or	a, a
	jp	z, __sdone_8365
	add	hl, hl
	djnz	__shift_2227
__sdone_8365:
	dec	sp
	dec	sp
	ld	-528(ix), l
	ld	-527(ix), h
	ld	l, -528(ix)
	ld	h, -527(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_9859
	ld	hl, #0
	jp	__cmp_e_81936
__cmp_t_9859:
	ld	hl, #1
__cmp_e_81936:
	dec	sp
	dec	sp
	ld	-530(ix), l
	ld	-529(ix), h
	ld	l, -530(ix)
	ld	h, -529(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51432
	ld	hl, #0
	jp	__cmp_e_52551
__cmp_t_51432:
	ld	hl, #1
__cmp_e_52551:
	dec	sp
	dec	sp
	ld	-532(ix), l
	ld	-531(ix), h
	ld	l, -532(ix)
	ld	h, -531(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L148
	jp	__xcc_L149
__xcc_L149:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-534(ix), l
	ld	-533(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-536(ix), l
	ld	-535(ix), h
	ld	l, -534(ix)
	ld	h, -533(ix)
	push	hl
	ld	l, -536(ix)
	ld	h, -535(ix)
	ld	b, l
	pop	hl
__shift_6437:
	ld	a, b
	or	a, a
	jp	z, __sdone_9228
	add	hl, hl
	djnz	__shift_6437
__sdone_9228:
	dec	sp
	dec	sp
	ld	-538(ix), l
	ld	-537(ix), h
	ld	l, -538(ix)
	ld	h, -537(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-540(ix), l
	ld	-539(ix), h
	ld	l, -540(ix)
	ld	h, -539(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_53275
	ld	hl, #0
	jp	__cmp_e_75407
__cmp_t_53275:
	ld	hl, #1
__cmp_e_75407:
	dec	sp
	dec	sp
	ld	-542(ix), l
	ld	-541(ix), h
	ld	l, -542(ix)
	ld	h, -541(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_1474
	ld	hl, #0
	jp	__cmp_e_76121
__cmp_t_1474:
	ld	hl, #1
__cmp_e_76121:
	dec	sp
	dec	sp
	ld	-544(ix), l
	ld	-543(ix), h
	jp	__xcc_L150
__xcc_L148:
	ld	hl, #1
	ld	-544(ix), l
	ld	-543(ix), h
__xcc_L150:
	ld	l, -544(ix)
	ld	h, -543(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L145
	jp	__xcc_L146
__xcc_L145:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-546(ix), l
	ld	-545(ix), h
	ld	l, -546(ix)
	ld	h, -545(ix)
	dec	sp
	dec	sp
	ld	-548(ix), l
	ld	-547(ix), h
	jp	__xcc_L147
__xcc_L146:
	ld	hl, #1
	ld	-548(ix), l
	ld	-547(ix), h
__xcc_L147:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-550(ix), l
	ld	-549(ix), h
	.globl __mul16
	ld	l, -550(ix)
	ld	h, -549(ix)
	push	hl
	ld	l, -548(ix)
	ld	h, -547(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-552(ix), l
	ld	-551(ix), h
	ld	l, -552(ix)
	ld	h, -551(ix)
	push	hl
	ld	l, -522(ix)
	ld	h, -521(ix)
	push	hl
	ld	l, -500(ix)
	ld	h, -499(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_151
	dec	sp
	dec	sp
	ld	-554(ix), l
	ld	-553(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-556(ix), l
	ld	-555(ix), h
	ld	l, -556(ix)
	ld	h, -555(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_68858
	ld	hl, #0
	jp	__cmp_e_94395
__cmp_t_68858:
	ld	hl, #1
__cmp_e_94395:
	dec	sp
	dec	sp
	ld	-558(ix), l
	ld	-557(ix), h
	ld	l, -558(ix)
	ld	h, -557(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_36029
	ld	hl, #0
	jp	__cmp_e_61237
__cmp_t_36029:
	ld	hl, #1
__cmp_e_61237:
	dec	sp
	dec	sp
	ld	-560(ix), l
	ld	-559(ix), h
	ld	l, -560(ix)
	ld	h, -559(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L155
	jp	__xcc_L156
__xcc_L156:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-562(ix), l
	ld	-561(ix), h
	ld	l, -562(ix)
	ld	h, -561(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-564(ix), l
	ld	-563(ix), h
	ld	l, -564(ix)
	ld	h, -563(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_8235
	ld	hl, #0
	jp	__cmp_e_73793
__cmp_t_8235:
	ld	hl, #1
__cmp_e_73793:
	dec	sp
	dec	sp
	ld	-566(ix), l
	ld	-565(ix), h
	ld	l, -566(ix)
	ld	h, -565(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65818
	ld	hl, #0
	jp	__cmp_e_94428
__cmp_t_65818:
	ld	hl, #1
__cmp_e_94428:
	dec	sp
	dec	sp
	ld	-568(ix), l
	ld	-567(ix), h
	jp	__xcc_L157
__xcc_L155:
	ld	hl, #1
	ld	-568(ix), l
	ld	-567(ix), h
__xcc_L157:
	ld	l, -568(ix)
	ld	h, -567(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L152
	jp	__xcc_L153
__xcc_L152:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-570(ix), l
	ld	-569(ix), h
	ld	l, -570(ix)
	ld	h, -569(ix)
	dec	sp
	dec	sp
	ld	-572(ix), l
	ld	-571(ix), h
	jp	__xcc_L154
__xcc_L153:
	ld	hl, #1
	ld	-572(ix), l
	ld	-571(ix), h
__xcc_L154:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-574(ix), l
	ld	-573(ix), h
	.globl __mul16
	ld	l, -574(ix)
	ld	h, -573(ix)
	push	hl
	ld	l, -572(ix)
	ld	h, -571(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-576(ix), l
	ld	-575(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-578(ix), l
	ld	-577(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-580(ix), l
	ld	-579(ix), h
	ld	l, -578(ix)
	ld	h, -577(ix)
	push	hl
	ld	l, -580(ix)
	ld	h, -579(ix)
	ld	b, l
	pop	hl
__shift_6143:
	ld	a, b
	or	a, a
	jp	z, __sdone_1011
	add	hl, hl
	djnz	__shift_6143
__sdone_1011:
	dec	sp
	dec	sp
	ld	-582(ix), l
	ld	-581(ix), h
	ld	l, -582(ix)
	ld	h, -581(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35928
	ld	hl, #0
	jp	__cmp_e_39529
__cmp_t_35928:
	ld	hl, #1
__cmp_e_39529:
	dec	sp
	dec	sp
	ld	-584(ix), l
	ld	-583(ix), h
	ld	l, -584(ix)
	ld	h, -583(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_18776
	ld	hl, #0
	jp	__cmp_e_22404
__cmp_t_18776:
	ld	hl, #1
__cmp_e_22404:
	dec	sp
	dec	sp
	ld	-586(ix), l
	ld	-585(ix), h
	ld	l, -586(ix)
	ld	h, -585(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L161
	jp	__xcc_L162
__xcc_L162:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-588(ix), l
	ld	-587(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-590(ix), l
	ld	-589(ix), h
	ld	l, -588(ix)
	ld	h, -587(ix)
	push	hl
	ld	l, -590(ix)
	ld	h, -589(ix)
	ld	b, l
	pop	hl
__shift_4443:
	ld	a, b
	or	a, a
	jp	z, __sdone_5763
	add	hl, hl
	djnz	__shift_4443
__sdone_5763:
	dec	sp
	dec	sp
	ld	-592(ix), l
	ld	-591(ix), h
	ld	l, -592(ix)
	ld	h, -591(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-594(ix), l
	ld	-593(ix), h
	ld	l, -594(ix)
	ld	h, -593(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_14613
	ld	hl, #0
	jp	__cmp_e_54538
__cmp_t_14613:
	ld	hl, #1
__cmp_e_54538:
	dec	sp
	dec	sp
	ld	-596(ix), l
	ld	-595(ix), h
	ld	l, -596(ix)
	ld	h, -595(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_18606
	ld	hl, #0
	jp	__cmp_e_36840
__cmp_t_18606:
	ld	hl, #1
__cmp_e_36840:
	dec	sp
	dec	sp
	ld	-598(ix), l
	ld	-597(ix), h
	jp	__xcc_L163
__xcc_L161:
	ld	hl, #1
	ld	-598(ix), l
	ld	-597(ix), h
__xcc_L163:
	ld	l, -598(ix)
	ld	h, -597(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L158
	jp	__xcc_L159
__xcc_L158:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-600(ix), l
	ld	-599(ix), h
	ld	l, -600(ix)
	ld	h, -599(ix)
	dec	sp
	dec	sp
	ld	-602(ix), l
	ld	-601(ix), h
	jp	__xcc_L160
__xcc_L159:
	ld	hl, #1
	ld	-602(ix), l
	ld	-601(ix), h
__xcc_L160:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-604(ix), l
	ld	-603(ix), h
	.globl __mul16
	ld	l, -604(ix)
	ld	h, -603(ix)
	push	hl
	ld	l, -602(ix)
	ld	h, -601(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-606(ix), l
	ld	-605(ix), h
	ld	l, -606(ix)
	ld	h, -605(ix)
	push	hl
	ld	l, -576(ix)
	ld	h, -575(ix)
	push	hl
	ld	l, -554(ix)
	ld	h, -553(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L136:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L135
	jp	__xcc_L137
__xcc_L137:
__xcc_L164:
	ld	hl, #__str_167
	dec	sp
	dec	sp
	ld	-608(ix), l
	ld	-607(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-610(ix), l
	ld	-609(ix), h
	ld	l, -610(ix)
	ld	h, -609(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_2904
	ld	hl, #0
	jp	__cmp_e_44818
__cmp_t_2904:
	ld	hl, #1
__cmp_e_44818:
	dec	sp
	dec	sp
	ld	-612(ix), l
	ld	-611(ix), h
	ld	l, -612(ix)
	ld	h, -611(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_35128
	ld	hl, #0
	jp	__cmp_e_70688
__cmp_t_35128:
	ld	hl, #1
__cmp_e_70688:
	dec	sp
	dec	sp
	ld	-614(ix), l
	ld	-613(ix), h
	ld	l, -614(ix)
	ld	h, -613(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L171
	jp	__xcc_L172
__xcc_L172:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-616(ix), l
	ld	-615(ix), h
	ld	l, -616(ix)
	ld	h, -615(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-618(ix), l
	ld	-617(ix), h
	ld	l, -618(ix)
	ld	h, -617(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97369
	ld	hl, #0
	jp	__cmp_e_67917
__cmp_t_97369:
	ld	hl, #1
__cmp_e_67917:
	dec	sp
	dec	sp
	ld	-620(ix), l
	ld	-619(ix), h
	ld	l, -620(ix)
	ld	h, -619(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_69917
	ld	hl, #0
	jp	__cmp_e_66996
__cmp_t_69917:
	ld	hl, #1
__cmp_e_66996:
	dec	sp
	dec	sp
	ld	-622(ix), l
	ld	-621(ix), h
	jp	__xcc_L173
__xcc_L171:
	ld	hl, #1
	ld	-622(ix), l
	ld	-621(ix), h
__xcc_L173:
	ld	l, -622(ix)
	ld	h, -621(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L168
	jp	__xcc_L169
__xcc_L168:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-624(ix), l
	ld	-623(ix), h
	ld	l, -624(ix)
	ld	h, -623(ix)
	dec	sp
	dec	sp
	ld	-626(ix), l
	ld	-625(ix), h
	jp	__xcc_L170
__xcc_L169:
	ld	hl, #1
	ld	-626(ix), l
	ld	-625(ix), h
__xcc_L170:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-628(ix), l
	ld	-627(ix), h
	.globl __mul16
	ld	l, -628(ix)
	ld	h, -627(ix)
	push	hl
	ld	l, -626(ix)
	ld	h, -625(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-630(ix), l
	ld	-629(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-632(ix), l
	ld	-631(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-634(ix), l
	ld	-633(ix), h
	ld	l, -632(ix)
	ld	h, -631(ix)
	push	hl
	ld	l, -634(ix)
	ld	h, -633(ix)
	ld	b, l
	pop	hl
__shift_3324:
	ld	a, b
	or	a, a
	jp	z, __sdone_7743
	add	hl, hl
	djnz	__shift_3324
__sdone_7743:
	dec	sp
	dec	sp
	ld	-636(ix), l
	ld	-635(ix), h
	ld	l, -636(ix)
	ld	h, -635(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59470
	ld	hl, #0
	jp	__cmp_e_12183
__cmp_t_59470:
	ld	hl, #1
__cmp_e_12183:
	dec	sp
	dec	sp
	ld	-638(ix), l
	ld	-637(ix), h
	ld	l, -638(ix)
	ld	h, -637(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98490
	ld	hl, #0
	jp	__cmp_e_95499
__cmp_t_98490:
	ld	hl, #1
__cmp_e_95499:
	dec	sp
	dec	sp
	ld	-640(ix), l
	ld	-639(ix), h
	ld	l, -640(ix)
	ld	h, -639(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L177
	jp	__xcc_L178
__xcc_L178:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-642(ix), l
	ld	-641(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-644(ix), l
	ld	-643(ix), h
	ld	l, -642(ix)
	ld	h, -641(ix)
	push	hl
	ld	l, -644(ix)
	ld	h, -643(ix)
	ld	b, l
	pop	hl
__shift_9772:
	ld	a, b
	or	a, a
	jp	z, __sdone_6725
	add	hl, hl
	djnz	__shift_9772
__sdone_6725:
	dec	sp
	dec	sp
	ld	-646(ix), l
	ld	-645(ix), h
	ld	l, -646(ix)
	ld	h, -645(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-648(ix), l
	ld	-647(ix), h
	ld	l, -648(ix)
	ld	h, -647(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85644
	ld	hl, #0
	jp	__cmp_e_55590
__cmp_t_85644:
	ld	hl, #1
__cmp_e_55590:
	dec	sp
	dec	sp
	ld	-650(ix), l
	ld	-649(ix), h
	ld	l, -650(ix)
	ld	h, -649(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17505
	ld	hl, #0
	jp	__cmp_e_68139
__cmp_t_17505:
	ld	hl, #1
__cmp_e_68139:
	dec	sp
	dec	sp
	ld	-652(ix), l
	ld	-651(ix), h
	jp	__xcc_L179
__xcc_L177:
	ld	hl, #1
	ld	-652(ix), l
	ld	-651(ix), h
__xcc_L179:
	ld	l, -652(ix)
	ld	h, -651(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L174
	jp	__xcc_L175
__xcc_L174:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-654(ix), l
	ld	-653(ix), h
	ld	l, -654(ix)
	ld	h, -653(ix)
	dec	sp
	dec	sp
	ld	-656(ix), l
	ld	-655(ix), h
	jp	__xcc_L176
__xcc_L175:
	ld	hl, #1
	ld	-656(ix), l
	ld	-655(ix), h
__xcc_L176:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-658(ix), l
	ld	-657(ix), h
	.globl __mul16
	ld	l, -658(ix)
	ld	h, -657(ix)
	push	hl
	ld	l, -656(ix)
	ld	h, -655(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-660(ix), l
	ld	-659(ix), h
	ld	l, -660(ix)
	ld	h, -659(ix)
	push	hl
	ld	l, -630(ix)
	ld	h, -629(ix)
	push	hl
	ld	l, -608(ix)
	ld	h, -607(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_180
	dec	sp
	dec	sp
	ld	-662(ix), l
	ld	-661(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-664(ix), l
	ld	-663(ix), h
	ld	l, -664(ix)
	ld	h, -663(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_2954
	ld	hl, #0
	jp	__cmp_e_69786
__cmp_t_2954:
	ld	hl, #1
__cmp_e_69786:
	dec	sp
	dec	sp
	ld	-666(ix), l
	ld	-665(ix), h
	ld	l, -666(ix)
	ld	h, -665(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7669
	ld	hl, #0
	jp	__cmp_e_38082
__cmp_t_7669:
	ld	hl, #1
__cmp_e_38082:
	dec	sp
	dec	sp
	ld	-668(ix), l
	ld	-667(ix), h
	ld	l, -668(ix)
	ld	h, -667(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L184
	jp	__xcc_L185
__xcc_L185:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-670(ix), l
	ld	-669(ix), h
	ld	l, -670(ix)
	ld	h, -669(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-672(ix), l
	ld	-671(ix), h
	ld	l, -672(ix)
	ld	h, -671(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_8542
	ld	hl, #0
	jp	__cmp_e_88464
__cmp_t_8542:
	ld	hl, #1
__cmp_e_88464:
	dec	sp
	dec	sp
	ld	-674(ix), l
	ld	-673(ix), h
	ld	l, -674(ix)
	ld	h, -673(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10197
	ld	hl, #0
	jp	__cmp_e_39507
__cmp_t_10197:
	ld	hl, #1
__cmp_e_39507:
	dec	sp
	dec	sp
	ld	-676(ix), l
	ld	-675(ix), h
	jp	__xcc_L186
__xcc_L184:
	ld	hl, #1
	ld	-676(ix), l
	ld	-675(ix), h
__xcc_L186:
	ld	l, -676(ix)
	ld	h, -675(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L181
	jp	__xcc_L182
__xcc_L181:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-678(ix), l
	ld	-677(ix), h
	ld	l, -678(ix)
	ld	h, -677(ix)
	dec	sp
	dec	sp
	ld	-680(ix), l
	ld	-679(ix), h
	jp	__xcc_L183
__xcc_L182:
	ld	hl, #1
	ld	-680(ix), l
	ld	-679(ix), h
__xcc_L183:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-682(ix), l
	ld	-681(ix), h
	.globl __mul16
	ld	l, -682(ix)
	ld	h, -681(ix)
	push	hl
	ld	l, -680(ix)
	ld	h, -679(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-684(ix), l
	ld	-683(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-686(ix), l
	ld	-685(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-688(ix), l
	ld	-687(ix), h
	ld	l, -686(ix)
	ld	h, -685(ix)
	push	hl
	ld	l, -688(ix)
	ld	h, -687(ix)
	ld	b, l
	pop	hl
__shift_9355:
	ld	a, b
	or	a, a
	jp	z, __sdone_8804
	add	hl, hl
	djnz	__shift_9355
__sdone_8804:
	dec	sp
	dec	sp
	ld	-690(ix), l
	ld	-689(ix), h
	ld	l, -690(ix)
	ld	h, -689(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_76348
	ld	hl, #0
	jp	__cmp_e_78611
__cmp_t_76348:
	ld	hl, #1
__cmp_e_78611:
	dec	sp
	dec	sp
	ld	-692(ix), l
	ld	-691(ix), h
	ld	l, -692(ix)
	ld	h, -691(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73622
	ld	hl, #0
	jp	__cmp_e_27828
__cmp_t_73622:
	ld	hl, #1
__cmp_e_27828:
	dec	sp
	dec	sp
	ld	-694(ix), l
	ld	-693(ix), h
	ld	l, -694(ix)
	ld	h, -693(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L190
	jp	__xcc_L191
__xcc_L191:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-696(ix), l
	ld	-695(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-698(ix), l
	ld	-697(ix), h
	ld	l, -696(ix)
	ld	h, -695(ix)
	push	hl
	ld	l, -698(ix)
	ld	h, -697(ix)
	ld	b, l
	pop	hl
__shift_9299:
	ld	a, b
	or	a, a
	jp	z, __sdone_7343
	add	hl, hl
	djnz	__shift_9299
__sdone_7343:
	dec	sp
	dec	sp
	ld	-700(ix), l
	ld	-699(ix), h
	ld	l, -700(ix)
	ld	h, -699(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-702(ix), l
	ld	-701(ix), h
	ld	l, -702(ix)
	ld	h, -701(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_95746
	ld	hl, #0
	jp	__cmp_e_35568
__cmp_t_95746:
	ld	hl, #1
__cmp_e_35568:
	dec	sp
	dec	sp
	ld	-704(ix), l
	ld	-703(ix), h
	ld	l, -704(ix)
	ld	h, -703(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_54340
	ld	hl, #0
	jp	__cmp_e_55422
__cmp_t_54340:
	ld	hl, #1
__cmp_e_55422:
	dec	sp
	dec	sp
	ld	-706(ix), l
	ld	-705(ix), h
	jp	__xcc_L192
__xcc_L190:
	ld	hl, #1
	ld	-706(ix), l
	ld	-705(ix), h
__xcc_L192:
	ld	l, -706(ix)
	ld	h, -705(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L187
	jp	__xcc_L188
__xcc_L187:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-708(ix), l
	ld	-707(ix), h
	ld	l, -708(ix)
	ld	h, -707(ix)
	dec	sp
	dec	sp
	ld	-710(ix), l
	ld	-709(ix), h
	jp	__xcc_L189
__xcc_L188:
	ld	hl, #1
	ld	-710(ix), l
	ld	-709(ix), h
__xcc_L189:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-712(ix), l
	ld	-711(ix), h
	.globl __mul16
	ld	l, -712(ix)
	ld	h, -711(ix)
	push	hl
	ld	l, -710(ix)
	ld	h, -709(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-714(ix), l
	ld	-713(ix), h
	ld	l, -714(ix)
	ld	h, -713(ix)
	push	hl
	ld	l, -684(ix)
	ld	h, -683(ix)
	push	hl
	ld	l, -662(ix)
	ld	h, -661(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L165:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L164
	jp	__xcc_L166
__xcc_L166:
__xcc_L193:
	ld	hl, #__str_196
	dec	sp
	dec	sp
	ld	-716(ix), l
	ld	-715(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-718(ix), l
	ld	-717(ix), h
	ld	l, -718(ix)
	ld	h, -717(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_23311
	ld	hl, #0
	jp	__cmp_e_13810
__cmp_t_23311:
	ld	hl, #1
__cmp_e_13810:
	dec	sp
	dec	sp
	ld	-720(ix), l
	ld	-719(ix), h
	ld	l, -720(ix)
	ld	h, -719(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_67605
	ld	hl, #0
	jp	__cmp_e_21801
__cmp_t_67605:
	ld	hl, #1
__cmp_e_21801:
	dec	sp
	dec	sp
	ld	-722(ix), l
	ld	-721(ix), h
	ld	l, -722(ix)
	ld	h, -721(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L200
	jp	__xcc_L201
__xcc_L201:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-724(ix), l
	ld	-723(ix), h
	ld	l, -724(ix)
	ld	h, -723(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-726(ix), l
	ld	-725(ix), h
	ld	l, -726(ix)
	ld	h, -725(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_25661
	ld	hl, #0
	jp	__cmp_e_73730
__cmp_t_25661:
	ld	hl, #1
__cmp_e_73730:
	dec	sp
	dec	sp
	ld	-728(ix), l
	ld	-727(ix), h
	ld	l, -728(ix)
	ld	h, -727(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44878
	ld	hl, #0
	jp	__cmp_e_11305
__cmp_t_44878:
	ld	hl, #1
__cmp_e_11305:
	dec	sp
	dec	sp
	ld	-730(ix), l
	ld	-729(ix), h
	jp	__xcc_L202
__xcc_L200:
	ld	hl, #1
	ld	-730(ix), l
	ld	-729(ix), h
__xcc_L202:
	ld	l, -730(ix)
	ld	h, -729(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L197
	jp	__xcc_L198
__xcc_L197:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-732(ix), l
	ld	-731(ix), h
	ld	l, -732(ix)
	ld	h, -731(ix)
	dec	sp
	dec	sp
	ld	-734(ix), l
	ld	-733(ix), h
	jp	__xcc_L199
__xcc_L198:
	ld	hl, #1
	ld	-734(ix), l
	ld	-733(ix), h
__xcc_L199:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-736(ix), l
	ld	-735(ix), h
	.globl __mul16
	ld	l, -736(ix)
	ld	h, -735(ix)
	push	hl
	ld	l, -734(ix)
	ld	h, -733(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-738(ix), l
	ld	-737(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-740(ix), l
	ld	-739(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-744(ix), l
	ld	-743(ix), h
	ld	l, -740(ix)
	ld	h, -739(ix)
	push	hl
	ld	l, -744(ix)
	ld	h, -743(ix)
	ld	b, l
	pop	hl
__shift_9320:
	ld	a, b
	or	a, a
	jp	z, __sdone_8736
	add	hl, hl
	djnz	__shift_9320
__sdone_8736:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-748(ix), l
	ld	-747(ix), h
	ld	l, -748(ix)
	ld	h, -747(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79444
	ld	hl, #0
	jp	__cmp_e_48626
__cmp_t_79444:
	ld	hl, #1
__cmp_e_48626:
	dec	sp
	dec	sp
	ld	-750(ix), l
	ld	-749(ix), h
	ld	l, -750(ix)
	ld	h, -749(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_48522
	ld	hl, #0
	jp	__cmp_e_3465
__cmp_t_48522:
	ld	hl, #1
__cmp_e_3465:
	dec	sp
	dec	sp
	ld	-752(ix), l
	ld	-751(ix), h
	ld	l, -752(ix)
	ld	h, -751(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L206
	jp	__xcc_L207
__xcc_L207:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-754(ix), l
	ld	-753(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-758(ix), l
	ld	-757(ix), h
	ld	l, -754(ix)
	ld	h, -753(ix)
	push	hl
	ld	l, -758(ix)
	ld	h, -757(ix)
	ld	b, l
	pop	hl
__shift_6708:
	ld	a, b
	or	a, a
	jp	z, __sdone_3416
	add	hl, hl
	djnz	__shift_6708
__sdone_3416:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-762(ix), l
	ld	-761(ix), h
	ld	l, -762(ix)
	ld	h, -761(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-764(ix), l
	ld	-763(ix), h
	ld	l, -764(ix)
	ld	h, -763(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_8282
	ld	hl, #0
	jp	__cmp_e_13258
__cmp_t_8282:
	ld	hl, #1
__cmp_e_13258:
	dec	sp
	dec	sp
	ld	-766(ix), l
	ld	-765(ix), h
	ld	l, -766(ix)
	ld	h, -765(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_12924
	ld	hl, #0
	jp	__cmp_e_67637
__cmp_t_12924:
	ld	hl, #1
__cmp_e_67637:
	dec	sp
	dec	sp
	ld	-768(ix), l
	ld	-767(ix), h
	jp	__xcc_L208
__xcc_L206:
	ld	hl, #1
	ld	-768(ix), l
	ld	-767(ix), h
__xcc_L208:
	ld	l, -768(ix)
	ld	h, -767(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L203
	jp	__xcc_L204
__xcc_L203:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-770(ix), l
	ld	-769(ix), h
	ld	l, -770(ix)
	ld	h, -769(ix)
	dec	sp
	dec	sp
	ld	-772(ix), l
	ld	-771(ix), h
	jp	__xcc_L205
__xcc_L204:
	ld	hl, #1
	ld	-772(ix), l
	ld	-771(ix), h
__xcc_L205:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-774(ix), l
	ld	-773(ix), h
	.globl __mul16
	ld	l, -774(ix)
	ld	h, -773(ix)
	push	hl
	ld	l, -772(ix)
	ld	h, -771(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-776(ix), l
	ld	-775(ix), h
	ld	l, -776(ix)
	ld	h, -775(ix)
	push	hl
	ld	l, -738(ix)
	ld	h, -737(ix)
	push	hl
	ld	l, -716(ix)
	ld	h, -715(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_209
	dec	sp
	dec	sp
	ld	-778(ix), l
	ld	-777(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-780(ix), l
	ld	-779(ix), h
	ld	l, -780(ix)
	ld	h, -779(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_42062
	ld	hl, #0
	jp	__cmp_e_5624
__cmp_t_42062:
	ld	hl, #1
__cmp_e_5624:
	dec	sp
	dec	sp
	ld	-782(ix), l
	ld	-781(ix), h
	ld	l, -782(ix)
	ld	h, -781(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_62600
	ld	hl, #0
	jp	__cmp_e_32036
__cmp_t_62600:
	ld	hl, #1
__cmp_e_32036:
	dec	sp
	dec	sp
	ld	-784(ix), l
	ld	-783(ix), h
	ld	l, -784(ix)
	ld	h, -783(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L213
	jp	__xcc_L214
__xcc_L214:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-786(ix), l
	ld	-785(ix), h
	ld	l, -786(ix)
	ld	h, -785(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-788(ix), l
	ld	-787(ix), h
	ld	l, -788(ix)
	ld	h, -787(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_33452
	ld	hl, #0
	jp	__cmp_e_11899
__cmp_t_33452:
	ld	hl, #1
__cmp_e_11899:
	dec	sp
	dec	sp
	ld	-790(ix), l
	ld	-789(ix), h
	ld	l, -790(ix)
	ld	h, -789(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19379
	ld	hl, #0
	jp	__cmp_e_45550
__cmp_t_19379:
	ld	hl, #1
__cmp_e_45550:
	dec	sp
	dec	sp
	ld	-792(ix), l
	ld	-791(ix), h
	jp	__xcc_L215
__xcc_L213:
	ld	hl, #1
	ld	-792(ix), l
	ld	-791(ix), h
__xcc_L215:
	ld	l, -792(ix)
	ld	h, -791(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L210
	jp	__xcc_L211
__xcc_L210:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-794(ix), l
	ld	-793(ix), h
	ld	l, -794(ix)
	ld	h, -793(ix)
	dec	sp
	dec	sp
	ld	-796(ix), l
	ld	-795(ix), h
	jp	__xcc_L212
__xcc_L211:
	ld	hl, #1
	ld	-796(ix), l
	ld	-795(ix), h
__xcc_L212:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-798(ix), l
	ld	-797(ix), h
	.globl __mul16
	ld	l, -798(ix)
	ld	h, -797(ix)
	push	hl
	ld	l, -796(ix)
	ld	h, -795(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-800(ix), l
	ld	-799(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-802(ix), l
	ld	-801(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-806(ix), l
	ld	-805(ix), h
	ld	l, -802(ix)
	ld	h, -801(ix)
	push	hl
	ld	l, -806(ix)
	ld	h, -805(ix)
	ld	b, l
	pop	hl
__shift_7468:
	ld	a, b
	or	a, a
	jp	z, __sdone_71
	add	hl, hl
	djnz	__shift_7468
__sdone_71:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-810(ix), l
	ld	-809(ix), h
	ld	l, -810(ix)
	ld	h, -809(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_973
	ld	hl, #0
	jp	__cmp_e_87131
__cmp_t_973:
	ld	hl, #1
__cmp_e_87131:
	dec	sp
	dec	sp
	ld	-812(ix), l
	ld	-811(ix), h
	ld	l, -812(ix)
	ld	h, -811(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3881
	ld	hl, #0
	jp	__cmp_e_84930
__cmp_t_3881:
	ld	hl, #1
__cmp_e_84930:
	dec	sp
	dec	sp
	ld	-814(ix), l
	ld	-813(ix), h
	ld	l, -814(ix)
	ld	h, -813(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L219
	jp	__xcc_L220
__xcc_L220:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-816(ix), l
	ld	-815(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-820(ix), l
	ld	-819(ix), h
	ld	l, -816(ix)
	ld	h, -815(ix)
	push	hl
	ld	l, -820(ix)
	ld	h, -819(ix)
	ld	b, l
	pop	hl
__shift_8933:
	ld	a, b
	or	a, a
	jp	z, __sdone_5894
	add	hl, hl
	djnz	__shift_8933
__sdone_5894:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-824(ix), l
	ld	-823(ix), h
	ld	l, -824(ix)
	ld	h, -823(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-826(ix), l
	ld	-825(ix), h
	ld	l, -826(ix)
	ld	h, -825(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_58660
	ld	hl, #0
	jp	__cmp_e_70163
__cmp_t_58660:
	ld	hl, #1
__cmp_e_70163:
	dec	sp
	dec	sp
	ld	-828(ix), l
	ld	-827(ix), h
	ld	l, -828(ix)
	ld	h, -827(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_57199
	ld	hl, #0
	jp	__cmp_e_87981
__cmp_t_57199:
	ld	hl, #1
__cmp_e_87981:
	dec	sp
	dec	sp
	ld	-830(ix), l
	ld	-829(ix), h
	jp	__xcc_L221
__xcc_L219:
	ld	hl, #1
	ld	-830(ix), l
	ld	-829(ix), h
__xcc_L221:
	ld	l, -830(ix)
	ld	h, -829(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L216
	jp	__xcc_L217
__xcc_L216:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-832(ix), l
	ld	-831(ix), h
	ld	l, -832(ix)
	ld	h, -831(ix)
	dec	sp
	dec	sp
	ld	-834(ix), l
	ld	-833(ix), h
	jp	__xcc_L218
__xcc_L217:
	ld	hl, #1
	ld	-834(ix), l
	ld	-833(ix), h
__xcc_L218:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-836(ix), l
	ld	-835(ix), h
	.globl __mul16
	ld	l, -836(ix)
	ld	h, -835(ix)
	push	hl
	ld	l, -834(ix)
	ld	h, -833(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-838(ix), l
	ld	-837(ix), h
	ld	l, -838(ix)
	ld	h, -837(ix)
	push	hl
	ld	l, -800(ix)
	ld	h, -799(ix)
	push	hl
	ld	l, -778(ix)
	ld	h, -777(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L194:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L193
	jp	__xcc_L195
__xcc_L195:
__xcc_L222:
	ld	hl, #__str_225
	dec	sp
	dec	sp
	ld	-840(ix), l
	ld	-839(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-842(ix), l
	ld	-841(ix), h
	ld	l, -842(ix)
	ld	h, -841(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_48899
	ld	hl, #0
	jp	__cmp_e_52996
__cmp_t_48899:
	ld	hl, #1
__cmp_e_52996:
	dec	sp
	dec	sp
	ld	-844(ix), l
	ld	-843(ix), h
	ld	l, -844(ix)
	ld	h, -843(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52959
	ld	hl, #0
	jp	__cmp_e_13773
__cmp_t_52959:
	ld	hl, #1
__cmp_e_13773:
	dec	sp
	dec	sp
	ld	-846(ix), l
	ld	-845(ix), h
	ld	l, -846(ix)
	ld	h, -845(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L229
	jp	__xcc_L230
__xcc_L230:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-848(ix), l
	ld	-847(ix), h
	ld	l, -848(ix)
	ld	h, -847(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-850(ix), l
	ld	-849(ix), h
	ld	l, -850(ix)
	ld	h, -849(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72813
	ld	hl, #0
	jp	__cmp_e_39668
__cmp_t_72813:
	ld	hl, #1
__cmp_e_39668:
	dec	sp
	dec	sp
	ld	-852(ix), l
	ld	-851(ix), h
	ld	l, -852(ix)
	ld	h, -851(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_87190
	ld	hl, #0
	jp	__cmp_e_81095
__cmp_t_87190:
	ld	hl, #1
__cmp_e_81095:
	dec	sp
	dec	sp
	ld	-854(ix), l
	ld	-853(ix), h
	jp	__xcc_L231
__xcc_L229:
	ld	hl, #1
	ld	-854(ix), l
	ld	-853(ix), h
__xcc_L231:
	ld	l, -854(ix)
	ld	h, -853(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L226
	jp	__xcc_L227
__xcc_L226:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-856(ix), l
	ld	-855(ix), h
	ld	l, -856(ix)
	ld	h, -855(ix)
	dec	sp
	dec	sp
	ld	-858(ix), l
	ld	-857(ix), h
	jp	__xcc_L228
__xcc_L227:
	ld	hl, #1
	ld	-858(ix), l
	ld	-857(ix), h
__xcc_L228:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-860(ix), l
	ld	-859(ix), h
	.globl __mul16
	ld	l, -860(ix)
	ld	h, -859(ix)
	push	hl
	ld	l, -858(ix)
	ld	h, -857(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-862(ix), l
	ld	-861(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-864(ix), l
	ld	-863(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-872(ix), l
	ld	-871(ix), h
	ld	l, -864(ix)
	ld	h, -863(ix)
	push	hl
	ld	l, -872(ix)
	ld	h, -871(ix)
	ld	b, l
	pop	hl
__shift_2926:
	ld	a, b
	or	a, a
	jp	z, __sdone_6466
	add	hl, hl
	djnz	__shift_2926
__sdone_6466:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-880(ix), l
	ld	-879(ix), h
	ld	l, -880(ix)
	ld	h, -879(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65084
	ld	hl, #0
	jp	__cmp_e_11340
__cmp_t_65084:
	ld	hl, #1
__cmp_e_11340:
	dec	sp
	dec	sp
	ld	-882(ix), l
	ld	-881(ix), h
	ld	l, -882(ix)
	ld	h, -881(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22090
	ld	hl, #0
	jp	__cmp_e_27684
__cmp_t_22090:
	ld	hl, #1
__cmp_e_27684:
	dec	sp
	dec	sp
	ld	-884(ix), l
	ld	-883(ix), h
	ld	l, -884(ix)
	ld	h, -883(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L235
	jp	__xcc_L236
__xcc_L236:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-886(ix), l
	ld	-885(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-894(ix), l
	ld	-893(ix), h
	ld	l, -886(ix)
	ld	h, -885(ix)
	push	hl
	ld	l, -894(ix)
	ld	h, -893(ix)
	ld	b, l
	pop	hl
__shift_3376:
	ld	a, b
	or	a, a
	jp	z, __sdone_5542
	add	hl, hl
	djnz	__shift_3376
__sdone_5542:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-902(ix), l
	ld	-901(ix), h
	ld	l, -902(ix)
	ld	h, -901(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-904(ix), l
	ld	-903(ix), h
	ld	l, -904(ix)
	ld	h, -903(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_55936
	ld	hl, #0
	jp	__cmp_e_79107
__cmp_t_55936:
	ld	hl, #1
__cmp_e_79107:
	dec	sp
	dec	sp
	ld	-906(ix), l
	ld	-905(ix), h
	ld	l, -906(ix)
	ld	h, -905(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17445
	ld	hl, #0
	jp	__cmp_e_19756
__cmp_t_17445:
	ld	hl, #1
__cmp_e_19756:
	dec	sp
	dec	sp
	ld	-908(ix), l
	ld	-907(ix), h
	jp	__xcc_L237
__xcc_L235:
	ld	hl, #1
	ld	-908(ix), l
	ld	-907(ix), h
__xcc_L237:
	ld	l, -908(ix)
	ld	h, -907(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L232
	jp	__xcc_L233
__xcc_L232:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-910(ix), l
	ld	-909(ix), h
	ld	l, -910(ix)
	ld	h, -909(ix)
	dec	sp
	dec	sp
	ld	-912(ix), l
	ld	-911(ix), h
	jp	__xcc_L234
__xcc_L233:
	ld	hl, #1
	ld	-912(ix), l
	ld	-911(ix), h
__xcc_L234:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-914(ix), l
	ld	-913(ix), h
	.globl __mul16
	ld	l, -914(ix)
	ld	h, -913(ix)
	push	hl
	ld	l, -912(ix)
	ld	h, -911(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-916(ix), l
	ld	-915(ix), h
	ld	l, -916(ix)
	ld	h, -915(ix)
	push	hl
	ld	l, -862(ix)
	ld	h, -861(ix)
	push	hl
	ld	l, -840(ix)
	ld	h, -839(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_238
	dec	sp
	dec	sp
	ld	-918(ix), l
	ld	-917(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-920(ix), l
	ld	-919(ix), h
	ld	l, -920(ix)
	ld	h, -919(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_69179
	ld	hl, #0
	jp	__cmp_e_18418
__cmp_t_69179:
	ld	hl, #1
__cmp_e_18418:
	dec	sp
	dec	sp
	ld	-922(ix), l
	ld	-921(ix), h
	ld	l, -922(ix)
	ld	h, -921(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6887
	ld	hl, #0
	jp	__cmp_e_89412
__cmp_t_6887:
	ld	hl, #1
__cmp_e_89412:
	dec	sp
	dec	sp
	ld	-924(ix), l
	ld	-923(ix), h
	ld	l, -924(ix)
	ld	h, -923(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L242
	jp	__xcc_L243
__xcc_L243:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-926(ix), l
	ld	-925(ix), h
	ld	l, -926(ix)
	ld	h, -925(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-928(ix), l
	ld	-927(ix), h
	ld	l, -928(ix)
	ld	h, -927(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3348
	ld	hl, #0
	jp	__cmp_e_32172
__cmp_t_3348:
	ld	hl, #1
__cmp_e_32172:
	dec	sp
	dec	sp
	ld	-930(ix), l
	ld	-929(ix), h
	ld	l, -930(ix)
	ld	h, -929(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51659
	ld	hl, #0
	jp	__cmp_e_62009
__cmp_t_51659:
	ld	hl, #1
__cmp_e_62009:
	dec	sp
	dec	sp
	ld	-932(ix), l
	ld	-931(ix), h
	jp	__xcc_L244
__xcc_L242:
	ld	hl, #1
	ld	-932(ix), l
	ld	-931(ix), h
__xcc_L244:
	ld	l, -932(ix)
	ld	h, -931(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L239
	jp	__xcc_L240
__xcc_L239:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-934(ix), l
	ld	-933(ix), h
	ld	l, -934(ix)
	ld	h, -933(ix)
	dec	sp
	dec	sp
	ld	-936(ix), l
	ld	-935(ix), h
	jp	__xcc_L241
__xcc_L240:
	ld	hl, #1
	ld	-936(ix), l
	ld	-935(ix), h
__xcc_L241:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-938(ix), l
	ld	-937(ix), h
	.globl __mul16
	ld	l, -938(ix)
	ld	h, -937(ix)
	push	hl
	ld	l, -936(ix)
	ld	h, -935(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-940(ix), l
	ld	-939(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-942(ix), l
	ld	-941(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-950(ix), l
	ld	-949(ix), h
	ld	l, -942(ix)
	ld	h, -941(ix)
	push	hl
	ld	l, -950(ix)
	ld	h, -949(ix)
	ld	b, l
	pop	hl
__shift_2336:
	ld	a, b
	or	a, a
	jp	z, __sdone_5210
	add	hl, hl
	djnz	__shift_2336
__sdone_5210:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-958(ix), l
	ld	-957(ix), h
	ld	l, -958(ix)
	ld	h, -957(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_66342
	ld	hl, #0
	jp	__cmp_e_67587
__cmp_t_66342:
	ld	hl, #1
__cmp_e_67587:
	dec	sp
	dec	sp
	ld	-960(ix), l
	ld	-959(ix), h
	ld	l, -960(ix)
	ld	h, -959(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_78206
	ld	hl, #0
	jp	__cmp_e_19301
__cmp_t_78206:
	ld	hl, #1
__cmp_e_19301:
	dec	sp
	dec	sp
	ld	-962(ix), l
	ld	-961(ix), h
	ld	l, -962(ix)
	ld	h, -961(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L248
	jp	__xcc_L249
__xcc_L249:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-964(ix), l
	ld	-963(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-972(ix), l
	ld	-971(ix), h
	ld	l, -964(ix)
	ld	h, -963(ix)
	push	hl
	ld	l, -972(ix)
	ld	h, -971(ix)
	ld	b, l
	pop	hl
__shift_7713:
	ld	a, b
	or	a, a
	jp	z, __sdone_7372
	add	hl, hl
	djnz	__shift_7713
__sdone_7372:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-980(ix), l
	ld	-979(ix), h
	ld	l, -980(ix)
	ld	h, -979(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-982(ix), l
	ld	-981(ix), h
	ld	l, -982(ix)
	ld	h, -981(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_75321
	ld	hl, #0
	jp	__cmp_e_1255
__cmp_t_75321:
	ld	hl, #1
__cmp_e_1255:
	dec	sp
	dec	sp
	ld	-984(ix), l
	ld	-983(ix), h
	ld	l, -984(ix)
	ld	h, -983(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_64819
	ld	hl, #0
	jp	__cmp_e_44599
__cmp_t_64819:
	ld	hl, #1
__cmp_e_44599:
	dec	sp
	dec	sp
	ld	-986(ix), l
	ld	-985(ix), h
	jp	__xcc_L250
__xcc_L248:
	ld	hl, #1
	ld	-986(ix), l
	ld	-985(ix), h
__xcc_L250:
	ld	l, -986(ix)
	ld	h, -985(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L245
	jp	__xcc_L246
__xcc_L245:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-988(ix), l
	ld	-987(ix), h
	ld	l, -988(ix)
	ld	h, -987(ix)
	dec	sp
	dec	sp
	ld	-990(ix), l
	ld	-989(ix), h
	jp	__xcc_L247
__xcc_L246:
	ld	hl, #1
	ld	-990(ix), l
	ld	-989(ix), h
__xcc_L247:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-992(ix), l
	ld	-991(ix), h
	.globl __mul16
	ld	l, -992(ix)
	ld	h, -991(ix)
	push	hl
	ld	l, -990(ix)
	ld	h, -989(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-994(ix), l
	ld	-993(ix), h
	ld	l, -994(ix)
	ld	h, -993(ix)
	push	hl
	ld	l, -940(ix)
	ld	h, -939(ix)
	push	hl
	ld	l, -918(ix)
	ld	h, -917(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L223:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L222
	jp	__xcc_L224
__xcc_L224:
__xcc_L133:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L132
	jp	__xcc_L134
__xcc_L134:
__xcc_L11:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	jp	__xcc_L12
__xcc_L12:
__xcc_L251:
__xcc_L254:
__xcc_L257:
	ld	hl, #__str_260
	dec	sp
	dec	sp
	ld	-996(ix), l
	ld	-995(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-998(ix), l
	ld	-997(ix), h
	ld	l, -998(ix)
	ld	h, -997(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_17721
	ld	hl, #0
	jp	__cmp_e_29904
__cmp_t_17721:
	ld	hl, #1
__cmp_e_29904:
	dec	sp
	dec	sp
	ld	-1000(ix), l
	ld	-999(ix), h
	ld	l, -1000(ix)
	ld	h, -999(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_55939
	ld	hl, #0
	jp	__cmp_e_39811
__cmp_t_55939:
	ld	hl, #1
__cmp_e_39811:
	dec	sp
	dec	sp
	ld	-1002(ix), l
	ld	-1001(ix), h
	ld	l, -1002(ix)
	ld	h, -1001(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L264
	jp	__xcc_L265
__xcc_L265:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1004(ix), l
	ld	-1003(ix), h
	ld	l, -1004(ix)
	ld	h, -1003(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1006(ix), l
	ld	-1005(ix), h
	ld	l, -1006(ix)
	ld	h, -1005(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73940
	ld	hl, #0
	jp	__cmp_e_15667
__cmp_t_73940:
	ld	hl, #1
__cmp_e_15667:
	dec	sp
	dec	sp
	ld	-1008(ix), l
	ld	-1007(ix), h
	ld	l, -1008(ix)
	ld	h, -1007(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_11705
	ld	hl, #0
	jp	__cmp_e_46228
__cmp_t_11705:
	ld	hl, #1
__cmp_e_46228:
	dec	sp
	dec	sp
	ld	-1010(ix), l
	ld	-1009(ix), h
	jp	__xcc_L266
__xcc_L264:
	ld	hl, #1
	ld	-1010(ix), l
	ld	-1009(ix), h
__xcc_L266:
	ld	l, -1010(ix)
	ld	h, -1009(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L261
	jp	__xcc_L262
__xcc_L261:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1012(ix), l
	ld	-1011(ix), h
	ld	l, -1012(ix)
	ld	h, -1011(ix)
	dec	sp
	dec	sp
	ld	-1014(ix), l
	ld	-1013(ix), h
	jp	__xcc_L263
__xcc_L262:
	ld	hl, #1
	ld	-1014(ix), l
	ld	-1013(ix), h
__xcc_L263:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1016(ix), l
	ld	-1015(ix), h
	.globl __mul16
	ld	l, -1016(ix)
	ld	h, -1015(ix)
	push	hl
	ld	l, -1014(ix)
	ld	h, -1013(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1018(ix), l
	ld	-1017(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1020(ix), l
	ld	-1019(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1022(ix), l
	ld	-1021(ix), h
	ld	l, -1020(ix)
	ld	h, -1019(ix)
	push	hl
	ld	l, -1022(ix)
	ld	h, -1021(ix)
	ld	b, l
	pop	hl
__shift_1127:
	ld	a, b
	or	a, a
	jp	z, __sdone_9150
	add	hl, hl
	djnz	__shift_1127
__sdone_9150:
	dec	sp
	dec	sp
	ld	-1024(ix), l
	ld	-1023(ix), h
	ld	l, -1024(ix)
	ld	h, -1023(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65984
	ld	hl, #0
	jp	__cmp_e_96658
__cmp_t_65984:
	ld	hl, #1
__cmp_e_96658:
	dec	sp
	dec	sp
	ld	-1026(ix), l
	ld	-1025(ix), h
	ld	l, -1026(ix)
	ld	h, -1025(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_63920
	ld	hl, #0
	jp	__cmp_e_89224
__cmp_t_63920:
	ld	hl, #1
__cmp_e_89224:
	dec	sp
	dec	sp
	ld	-1028(ix), l
	ld	-1027(ix), h
	ld	l, -1028(ix)
	ld	h, -1027(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L270
	jp	__xcc_L271
__xcc_L271:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1030(ix), l
	ld	-1029(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1032(ix), l
	ld	-1031(ix), h
	ld	l, -1030(ix)
	ld	h, -1029(ix)
	push	hl
	ld	l, -1032(ix)
	ld	h, -1031(ix)
	ld	b, l
	pop	hl
__shift_2422:
	ld	a, b
	or	a, a
	jp	z, __sdone_7269
	add	hl, hl
	djnz	__shift_2422
__sdone_7269:
	dec	sp
	dec	sp
	ld	-1034(ix), l
	ld	-1033(ix), h
	ld	l, -1034(ix)
	ld	h, -1033(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1036(ix), l
	ld	-1035(ix), h
	ld	l, -1036(ix)
	ld	h, -1035(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_21396
	ld	hl, #0
	jp	__cmp_e_54081
__cmp_t_21396:
	ld	hl, #1
__cmp_e_54081:
	dec	sp
	dec	sp
	ld	-1038(ix), l
	ld	-1037(ix), h
	ld	l, -1038(ix)
	ld	h, -1037(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_45630
	ld	hl, #0
	jp	__cmp_e_40084
__cmp_t_45630:
	ld	hl, #1
__cmp_e_40084:
	dec	sp
	dec	sp
	ld	-1040(ix), l
	ld	-1039(ix), h
	jp	__xcc_L272
__xcc_L270:
	ld	hl, #1
	ld	-1040(ix), l
	ld	-1039(ix), h
__xcc_L272:
	ld	l, -1040(ix)
	ld	h, -1039(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L267
	jp	__xcc_L268
__xcc_L267:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1042(ix), l
	ld	-1041(ix), h
	ld	l, -1042(ix)
	ld	h, -1041(ix)
	dec	sp
	dec	sp
	ld	-1044(ix), l
	ld	-1043(ix), h
	jp	__xcc_L269
__xcc_L268:
	ld	hl, #1
	ld	-1044(ix), l
	ld	-1043(ix), h
__xcc_L269:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1046(ix), l
	ld	-1045(ix), h
	.globl __mul16
	ld	l, -1046(ix)
	ld	h, -1045(ix)
	push	hl
	ld	l, -1044(ix)
	ld	h, -1043(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1048(ix), l
	ld	-1047(ix), h
	ld	l, -1048(ix)
	ld	h, -1047(ix)
	push	hl
	ld	l, -1018(ix)
	ld	h, -1017(ix)
	push	hl
	ld	l, -996(ix)
	ld	h, -995(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_273
	dec	sp
	dec	sp
	ld	-1050(ix), l
	ld	-1049(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1052(ix), l
	ld	-1051(ix), h
	ld	l, -1052(ix)
	ld	h, -1051(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79292
	ld	hl, #0
	jp	__cmp_e_11972
__cmp_t_79292:
	ld	hl, #1
__cmp_e_11972:
	dec	sp
	dec	sp
	ld	-1054(ix), l
	ld	-1053(ix), h
	ld	l, -1054(ix)
	ld	h, -1053(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7672
	ld	hl, #0
	jp	__cmp_e_73850
__cmp_t_7672:
	ld	hl, #1
__cmp_e_73850:
	dec	sp
	dec	sp
	ld	-1056(ix), l
	ld	-1055(ix), h
	ld	l, -1056(ix)
	ld	h, -1055(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L277
	jp	__xcc_L278
__xcc_L278:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1058(ix), l
	ld	-1057(ix), h
	ld	l, -1058(ix)
	ld	h, -1057(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1060(ix), l
	ld	-1059(ix), h
	ld	l, -1060(ix)
	ld	h, -1059(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47625
	ld	hl, #0
	jp	__cmp_e_5385
__cmp_t_47625:
	ld	hl, #1
__cmp_e_5385:
	dec	sp
	dec	sp
	ld	-1062(ix), l
	ld	-1061(ix), h
	ld	l, -1062(ix)
	ld	h, -1061(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41222
	ld	hl, #0
	jp	__cmp_e_39299
__cmp_t_41222:
	ld	hl, #1
__cmp_e_39299:
	dec	sp
	dec	sp
	ld	-1064(ix), l
	ld	-1063(ix), h
	jp	__xcc_L279
__xcc_L277:
	ld	hl, #1
	ld	-1064(ix), l
	ld	-1063(ix), h
__xcc_L279:
	ld	l, -1064(ix)
	ld	h, -1063(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L274
	jp	__xcc_L275
__xcc_L274:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1066(ix), l
	ld	-1065(ix), h
	ld	l, -1066(ix)
	ld	h, -1065(ix)
	dec	sp
	dec	sp
	ld	-1068(ix), l
	ld	-1067(ix), h
	jp	__xcc_L276
__xcc_L275:
	ld	hl, #1
	ld	-1068(ix), l
	ld	-1067(ix), h
__xcc_L276:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1070(ix), l
	ld	-1069(ix), h
	.globl __mul16
	ld	l, -1070(ix)
	ld	h, -1069(ix)
	push	hl
	ld	l, -1068(ix)
	ld	h, -1067(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1072(ix), l
	ld	-1071(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1074(ix), l
	ld	-1073(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1076(ix), l
	ld	-1075(ix), h
	ld	l, -1074(ix)
	ld	h, -1073(ix)
	push	hl
	ld	l, -1076(ix)
	ld	h, -1075(ix)
	ld	b, l
	pop	hl
__shift_6640:
	ld	a, b
	or	a, a
	jp	z, __sdone_6042
	add	hl, hl
	djnz	__shift_6640
__sdone_6042:
	dec	sp
	dec	sp
	ld	-1078(ix), l
	ld	-1077(ix), h
	ld	l, -1078(ix)
	ld	h, -1077(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_83898
	ld	hl, #0
	jp	__cmp_e_40713
__cmp_t_83898:
	ld	hl, #1
__cmp_e_40713:
	dec	sp
	dec	sp
	ld	-1080(ix), l
	ld	-1079(ix), h
	ld	l, -1080(ix)
	ld	h, -1079(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52298
	ld	hl, #0
	jp	__cmp_e_56190
__cmp_t_52298:
	ld	hl, #1
__cmp_e_56190:
	dec	sp
	dec	sp
	ld	-1082(ix), l
	ld	-1081(ix), h
	ld	l, -1082(ix)
	ld	h, -1081(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L283
	jp	__xcc_L284
__xcc_L284:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1084(ix), l
	ld	-1083(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1086(ix), l
	ld	-1085(ix), h
	ld	l, -1084(ix)
	ld	h, -1083(ix)
	push	hl
	ld	l, -1086(ix)
	ld	h, -1085(ix)
	ld	b, l
	pop	hl
__shift_524:
	ld	a, b
	or	a, a
	jp	z, __sdone_2590
	add	hl, hl
	djnz	__shift_524
__sdone_2590:
	dec	sp
	dec	sp
	ld	-1088(ix), l
	ld	-1087(ix), h
	ld	l, -1088(ix)
	ld	h, -1087(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1090(ix), l
	ld	-1089(ix), h
	ld	l, -1090(ix)
	ld	h, -1089(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_88209
	ld	hl, #0
	jp	__cmp_e_8581
__cmp_t_88209:
	ld	hl, #1
__cmp_e_8581:
	dec	sp
	dec	sp
	ld	-1092(ix), l
	ld	-1091(ix), h
	ld	l, -1092(ix)
	ld	h, -1091(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88819
	ld	hl, #0
	jp	__cmp_e_99336
__cmp_t_88819:
	ld	hl, #1
__cmp_e_99336:
	dec	sp
	dec	sp
	ld	-1094(ix), l
	ld	-1093(ix), h
	jp	__xcc_L285
__xcc_L283:
	ld	hl, #1
	ld	-1094(ix), l
	ld	-1093(ix), h
__xcc_L285:
	ld	l, -1094(ix)
	ld	h, -1093(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L280
	jp	__xcc_L281
__xcc_L280:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1096(ix), l
	ld	-1095(ix), h
	ld	l, -1096(ix)
	ld	h, -1095(ix)
	dec	sp
	dec	sp
	ld	-1098(ix), l
	ld	-1097(ix), h
	jp	__xcc_L282
__xcc_L281:
	ld	hl, #1
	ld	-1098(ix), l
	ld	-1097(ix), h
__xcc_L282:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1100(ix), l
	ld	-1099(ix), h
	.globl __mul16
	ld	l, -1100(ix)
	ld	h, -1099(ix)
	push	hl
	ld	l, -1098(ix)
	ld	h, -1097(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1102(ix), l
	ld	-1101(ix), h
	ld	l, -1102(ix)
	ld	h, -1101(ix)
	push	hl
	ld	l, -1072(ix)
	ld	h, -1071(ix)
	push	hl
	ld	l, -1050(ix)
	ld	h, -1049(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L258:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L257
	jp	__xcc_L259
__xcc_L259:
__xcc_L286:
	ld	hl, #__str_289
	dec	sp
	dec	sp
	ld	-1104(ix), l
	ld	-1103(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1106(ix), l
	ld	-1105(ix), h
	ld	l, -1106(ix)
	ld	h, -1105(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37732
	ld	hl, #0
	jp	__cmp_e_71155
__cmp_t_37732:
	ld	hl, #1
__cmp_e_71155:
	dec	sp
	dec	sp
	ld	-1108(ix), l
	ld	-1107(ix), h
	ld	l, -1108(ix)
	ld	h, -1107(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95994
	ld	hl, #0
	jp	__cmp_e_18004
__cmp_t_95994:
	ld	hl, #1
__cmp_e_18004:
	dec	sp
	dec	sp
	ld	-1110(ix), l
	ld	-1109(ix), h
	ld	l, -1110(ix)
	ld	h, -1109(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L293
	jp	__xcc_L294
__xcc_L294:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1112(ix), l
	ld	-1111(ix), h
	ld	l, -1112(ix)
	ld	h, -1111(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1114(ix), l
	ld	-1113(ix), h
	ld	l, -1114(ix)
	ld	h, -1113(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60379
	ld	hl, #0
	jp	__cmp_e_14769
__cmp_t_60379:
	ld	hl, #1
__cmp_e_14769:
	dec	sp
	dec	sp
	ld	-1116(ix), l
	ld	-1115(ix), h
	ld	l, -1116(ix)
	ld	h, -1115(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85273
	ld	hl, #0
	jp	__cmp_e_81776
__cmp_t_85273:
	ld	hl, #1
__cmp_e_81776:
	dec	sp
	dec	sp
	ld	-1118(ix), l
	ld	-1117(ix), h
	jp	__xcc_L295
__xcc_L293:
	ld	hl, #1
	ld	-1118(ix), l
	ld	-1117(ix), h
__xcc_L295:
	ld	l, -1118(ix)
	ld	h, -1117(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L290
	jp	__xcc_L291
__xcc_L290:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1120(ix), l
	ld	-1119(ix), h
	ld	l, -1120(ix)
	ld	h, -1119(ix)
	dec	sp
	dec	sp
	ld	-1122(ix), l
	ld	-1121(ix), h
	jp	__xcc_L292
__xcc_L291:
	ld	hl, #1
	ld	-1122(ix), l
	ld	-1121(ix), h
__xcc_L292:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1124(ix), l
	ld	-1123(ix), h
	.globl __mul16
	ld	l, -1124(ix)
	ld	h, -1123(ix)
	push	hl
	ld	l, -1122(ix)
	ld	h, -1121(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1126(ix), l
	ld	-1125(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1128(ix), l
	ld	-1127(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1130(ix), l
	ld	-1129(ix), h
	ld	l, -1128(ix)
	ld	h, -1127(ix)
	push	hl
	ld	l, -1130(ix)
	ld	h, -1129(ix)
	ld	b, l
	pop	hl
__shift_8850:
	ld	a, b
	or	a, a
	jp	z, __sdone_7255
	add	hl, hl
	djnz	__shift_8850
__sdone_7255:
	dec	sp
	dec	sp
	ld	-1132(ix), l
	ld	-1131(ix), h
	ld	l, -1132(ix)
	ld	h, -1131(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_21860
	ld	hl, #0
	jp	__cmp_e_48142
__cmp_t_21860:
	ld	hl, #1
__cmp_e_48142:
	dec	sp
	dec	sp
	ld	-1134(ix), l
	ld	-1133(ix), h
	ld	l, -1134(ix)
	ld	h, -1133(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75579
	ld	hl, #0
	jp	__cmp_e_45884
__cmp_t_75579:
	ld	hl, #1
__cmp_e_45884:
	dec	sp
	dec	sp
	ld	-1136(ix), l
	ld	-1135(ix), h
	ld	l, -1136(ix)
	ld	h, -1135(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L299
	jp	__xcc_L300
__xcc_L300:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1138(ix), l
	ld	-1137(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1140(ix), l
	ld	-1139(ix), h
	ld	l, -1138(ix)
	ld	h, -1137(ix)
	push	hl
	ld	l, -1140(ix)
	ld	h, -1139(ix)
	ld	b, l
	pop	hl
__shift_1993:
	ld	a, b
	or	a, a
	jp	z, __sdone_3205
	add	hl, hl
	djnz	__shift_1993
__sdone_3205:
	dec	sp
	dec	sp
	ld	-1142(ix), l
	ld	-1141(ix), h
	ld	l, -1142(ix)
	ld	h, -1141(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1144(ix), l
	ld	-1143(ix), h
	ld	l, -1144(ix)
	ld	h, -1143(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_67621
	ld	hl, #0
	jp	__cmp_e_79567
__cmp_t_67621:
	ld	hl, #1
__cmp_e_79567:
	dec	sp
	dec	sp
	ld	-1146(ix), l
	ld	-1145(ix), h
	ld	l, -1146(ix)
	ld	h, -1145(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_62504
	ld	hl, #0
	jp	__cmp_e_90613
__cmp_t_62504:
	ld	hl, #1
__cmp_e_90613:
	dec	sp
	dec	sp
	ld	-1148(ix), l
	ld	-1147(ix), h
	jp	__xcc_L301
__xcc_L299:
	ld	hl, #1
	ld	-1148(ix), l
	ld	-1147(ix), h
__xcc_L301:
	ld	l, -1148(ix)
	ld	h, -1147(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L296
	jp	__xcc_L297
__xcc_L296:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1150(ix), l
	ld	-1149(ix), h
	ld	l, -1150(ix)
	ld	h, -1149(ix)
	dec	sp
	dec	sp
	ld	-1152(ix), l
	ld	-1151(ix), h
	jp	__xcc_L298
__xcc_L297:
	ld	hl, #1
	ld	-1152(ix), l
	ld	-1151(ix), h
__xcc_L298:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1154(ix), l
	ld	-1153(ix), h
	.globl __mul16
	ld	l, -1154(ix)
	ld	h, -1153(ix)
	push	hl
	ld	l, -1152(ix)
	ld	h, -1151(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1156(ix), l
	ld	-1155(ix), h
	ld	l, -1156(ix)
	ld	h, -1155(ix)
	push	hl
	ld	l, -1126(ix)
	ld	h, -1125(ix)
	push	hl
	ld	l, -1104(ix)
	ld	h, -1103(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_302
	dec	sp
	dec	sp
	ld	-1158(ix), l
	ld	-1157(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1160(ix), l
	ld	-1159(ix), h
	ld	l, -1160(ix)
	ld	h, -1159(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1961
	ld	hl, #0
	jp	__cmp_e_62754
__cmp_t_1961:
	ld	hl, #1
__cmp_e_62754:
	dec	sp
	dec	sp
	ld	-1162(ix), l
	ld	-1161(ix), h
	ld	l, -1162(ix)
	ld	h, -1161(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_31326
	ld	hl, #0
	jp	__cmp_e_54259
__cmp_t_31326:
	ld	hl, #1
__cmp_e_54259:
	dec	sp
	dec	sp
	ld	-1164(ix), l
	ld	-1163(ix), h
	ld	l, -1164(ix)
	ld	h, -1163(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L306
	jp	__xcc_L307
__xcc_L307:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1166(ix), l
	ld	-1165(ix), h
	ld	l, -1166(ix)
	ld	h, -1165(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1168(ix), l
	ld	-1167(ix), h
	ld	l, -1168(ix)
	ld	h, -1167(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18944
	ld	hl, #0
	jp	__cmp_e_28202
__cmp_t_18944:
	ld	hl, #1
__cmp_e_28202:
	dec	sp
	dec	sp
	ld	-1170(ix), l
	ld	-1169(ix), h
	ld	l, -1170(ix)
	ld	h, -1169(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13202
	ld	hl, #0
	jp	__cmp_e_23506
__cmp_t_13202:
	ld	hl, #1
__cmp_e_23506:
	dec	sp
	dec	sp
	ld	-1172(ix), l
	ld	-1171(ix), h
	jp	__xcc_L308
__xcc_L306:
	ld	hl, #1
	ld	-1172(ix), l
	ld	-1171(ix), h
__xcc_L308:
	ld	l, -1172(ix)
	ld	h, -1171(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L303
	jp	__xcc_L304
__xcc_L303:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1174(ix), l
	ld	-1173(ix), h
	ld	l, -1174(ix)
	ld	h, -1173(ix)
	dec	sp
	dec	sp
	ld	-1176(ix), l
	ld	-1175(ix), h
	jp	__xcc_L305
__xcc_L304:
	ld	hl, #1
	ld	-1176(ix), l
	ld	-1175(ix), h
__xcc_L305:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1178(ix), l
	ld	-1177(ix), h
	.globl __mul16
	ld	l, -1178(ix)
	ld	h, -1177(ix)
	push	hl
	ld	l, -1176(ix)
	ld	h, -1175(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1180(ix), l
	ld	-1179(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1182(ix), l
	ld	-1181(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1184(ix), l
	ld	-1183(ix), h
	ld	l, -1182(ix)
	ld	h, -1181(ix)
	push	hl
	ld	l, -1184(ix)
	ld	h, -1183(ix)
	ld	b, l
	pop	hl
__shift_6784:
	ld	a, b
	or	a, a
	jp	z, __sdone_2021
	add	hl, hl
	djnz	__shift_6784
__sdone_2021:
	dec	sp
	dec	sp
	ld	-1186(ix), l
	ld	-1185(ix), h
	ld	l, -1186(ix)
	ld	h, -1185(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22842
	ld	hl, #0
	jp	__cmp_e_90868
__cmp_t_22842:
	ld	hl, #1
__cmp_e_90868:
	dec	sp
	dec	sp
	ld	-1188(ix), l
	ld	-1187(ix), h
	ld	l, -1188(ix)
	ld	h, -1187(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89528
	ld	hl, #0
	jp	__cmp_e_35189
__cmp_t_89528:
	ld	hl, #1
__cmp_e_35189:
	dec	sp
	dec	sp
	ld	-1190(ix), l
	ld	-1189(ix), h
	ld	l, -1190(ix)
	ld	h, -1189(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L312
	jp	__xcc_L313
__xcc_L313:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1192(ix), l
	ld	-1191(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1194(ix), l
	ld	-1193(ix), h
	ld	l, -1192(ix)
	ld	h, -1191(ix)
	push	hl
	ld	l, -1194(ix)
	ld	h, -1193(ix)
	ld	b, l
	pop	hl
__shift_8872:
	ld	a, b
	or	a, a
	jp	z, __sdone_9908
	add	hl, hl
	djnz	__shift_8872
__sdone_9908:
	dec	sp
	dec	sp
	ld	-1196(ix), l
	ld	-1195(ix), h
	ld	l, -1196(ix)
	ld	h, -1195(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1198(ix), l
	ld	-1197(ix), h
	ld	l, -1198(ix)
	ld	h, -1197(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49958
	ld	hl, #0
	jp	__cmp_e_10498
__cmp_t_49958:
	ld	hl, #1
__cmp_e_10498:
	dec	sp
	dec	sp
	ld	-1200(ix), l
	ld	-1199(ix), h
	ld	l, -1200(ix)
	ld	h, -1199(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_48036
	ld	hl, #0
	jp	__cmp_e_18808
__cmp_t_48036:
	ld	hl, #1
__cmp_e_18808:
	dec	sp
	dec	sp
	ld	-1202(ix), l
	ld	-1201(ix), h
	jp	__xcc_L314
__xcc_L312:
	ld	hl, #1
	ld	-1202(ix), l
	ld	-1201(ix), h
__xcc_L314:
	ld	l, -1202(ix)
	ld	h, -1201(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L309
	jp	__xcc_L310
__xcc_L309:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1204(ix), l
	ld	-1203(ix), h
	ld	l, -1204(ix)
	ld	h, -1203(ix)
	dec	sp
	dec	sp
	ld	-1206(ix), l
	ld	-1205(ix), h
	jp	__xcc_L311
__xcc_L310:
	ld	hl, #1
	ld	-1206(ix), l
	ld	-1205(ix), h
__xcc_L311:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1208(ix), l
	ld	-1207(ix), h
	.globl __mul16
	ld	l, -1208(ix)
	ld	h, -1207(ix)
	push	hl
	ld	l, -1206(ix)
	ld	h, -1205(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1210(ix), l
	ld	-1209(ix), h
	ld	l, -1210(ix)
	ld	h, -1209(ix)
	push	hl
	ld	l, -1180(ix)
	ld	h, -1179(ix)
	push	hl
	ld	l, -1158(ix)
	ld	h, -1157(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L287:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L286
	jp	__xcc_L288
__xcc_L288:
__xcc_L315:
	ld	hl, #__str_318
	dec	sp
	dec	sp
	ld	-1212(ix), l
	ld	-1211(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1214(ix), l
	ld	-1213(ix), h
	ld	l, -1214(ix)
	ld	h, -1213(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_57753
	ld	hl, #0
	jp	__cmp_e_86248
__cmp_t_57753:
	ld	hl, #1
__cmp_e_86248:
	dec	sp
	dec	sp
	ld	-1216(ix), l
	ld	-1215(ix), h
	ld	l, -1216(ix)
	ld	h, -1215(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_83303
	ld	hl, #0
	jp	__cmp_e_33333
__cmp_t_83303:
	ld	hl, #1
__cmp_e_33333:
	dec	sp
	dec	sp
	ld	-1218(ix), l
	ld	-1217(ix), h
	ld	l, -1218(ix)
	ld	h, -1217(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L322
	jp	__xcc_L323
__xcc_L323:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1220(ix), l
	ld	-1219(ix), h
	ld	l, -1220(ix)
	ld	h, -1219(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1222(ix), l
	ld	-1221(ix), h
	ld	l, -1222(ix)
	ld	h, -1221(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_32133
	ld	hl, #0
	jp	__cmp_e_21648
__cmp_t_32133:
	ld	hl, #1
__cmp_e_21648:
	dec	sp
	dec	sp
	ld	-1224(ix), l
	ld	-1223(ix), h
	ld	l, -1224(ix)
	ld	h, -1223(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_72890
	ld	hl, #0
	jp	__cmp_e_99754
__cmp_t_72890:
	ld	hl, #1
__cmp_e_99754:
	dec	sp
	dec	sp
	ld	-1226(ix), l
	ld	-1225(ix), h
	jp	__xcc_L324
__xcc_L322:
	ld	hl, #1
	ld	-1226(ix), l
	ld	-1225(ix), h
__xcc_L324:
	ld	l, -1226(ix)
	ld	h, -1225(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L319
	jp	__xcc_L320
__xcc_L319:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1228(ix), l
	ld	-1227(ix), h
	ld	l, -1228(ix)
	ld	h, -1227(ix)
	dec	sp
	dec	sp
	ld	-1230(ix), l
	ld	-1229(ix), h
	jp	__xcc_L321
__xcc_L320:
	ld	hl, #1
	ld	-1230(ix), l
	ld	-1229(ix), h
__xcc_L321:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1232(ix), l
	ld	-1231(ix), h
	.globl __mul16
	ld	l, -1232(ix)
	ld	h, -1231(ix)
	push	hl
	ld	l, -1230(ix)
	ld	h, -1229(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1234(ix), l
	ld	-1233(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1236(ix), l
	ld	-1235(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1240(ix), l
	ld	-1239(ix), h
	ld	l, -1236(ix)
	ld	h, -1235(ix)
	push	hl
	ld	l, -1240(ix)
	ld	h, -1239(ix)
	ld	b, l
	pop	hl
__shift_7567:
	ld	a, b
	or	a, a
	jp	z, __sdone_1746
	add	hl, hl
	djnz	__shift_7567
__sdone_1746:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1244(ix), l
	ld	-1243(ix), h
	ld	l, -1244(ix)
	ld	h, -1243(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90368
	ld	hl, #0
	jp	__cmp_e_19529
__cmp_t_90368:
	ld	hl, #1
__cmp_e_19529:
	dec	sp
	dec	sp
	ld	-1246(ix), l
	ld	-1245(ix), h
	ld	l, -1246(ix)
	ld	h, -1245(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14500
	ld	hl, #0
	jp	__cmp_e_38046
__cmp_t_14500:
	ld	hl, #1
__cmp_e_38046:
	dec	sp
	dec	sp
	ld	-1248(ix), l
	ld	-1247(ix), h
	ld	l, -1248(ix)
	ld	h, -1247(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L328
	jp	__xcc_L329
__xcc_L329:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1250(ix), l
	ld	-1249(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1254(ix), l
	ld	-1253(ix), h
	ld	l, -1250(ix)
	ld	h, -1249(ix)
	push	hl
	ld	l, -1254(ix)
	ld	h, -1253(ix)
	ld	b, l
	pop	hl
__shift_3788:
	ld	a, b
	or	a, a
	jp	z, __sdone_9797
	add	hl, hl
	djnz	__shift_3788
__sdone_9797:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1258(ix), l
	ld	-1257(ix), h
	ld	l, -1258(ix)
	ld	h, -1257(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1260(ix), l
	ld	-1259(ix), h
	ld	l, -1260(ix)
	ld	h, -1259(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_66249
	ld	hl, #0
	jp	__cmp_e_86990
__cmp_t_66249:
	ld	hl, #1
__cmp_e_86990:
	dec	sp
	dec	sp
	ld	-1262(ix), l
	ld	-1261(ix), h
	ld	l, -1262(ix)
	ld	h, -1261(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73303
	ld	hl, #0
	jp	__cmp_e_3033
__cmp_t_73303:
	ld	hl, #1
__cmp_e_3033:
	dec	sp
	dec	sp
	ld	-1264(ix), l
	ld	-1263(ix), h
	jp	__xcc_L330
__xcc_L328:
	ld	hl, #1
	ld	-1264(ix), l
	ld	-1263(ix), h
__xcc_L330:
	ld	l, -1264(ix)
	ld	h, -1263(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L325
	jp	__xcc_L326
__xcc_L325:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1266(ix), l
	ld	-1265(ix), h
	ld	l, -1266(ix)
	ld	h, -1265(ix)
	dec	sp
	dec	sp
	ld	-1268(ix), l
	ld	-1267(ix), h
	jp	__xcc_L327
__xcc_L326:
	ld	hl, #1
	ld	-1268(ix), l
	ld	-1267(ix), h
__xcc_L327:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-1270(ix), l
	ld	-1269(ix), h
	.globl __mul16
	ld	l, -1270(ix)
	ld	h, -1269(ix)
	push	hl
	ld	l, -1268(ix)
	ld	h, -1267(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1272(ix), l
	ld	-1271(ix), h
	ld	l, -1272(ix)
	ld	h, -1271(ix)
	push	hl
	ld	l, -1234(ix)
	ld	h, -1233(ix)
	push	hl
	ld	l, -1212(ix)
	ld	h, -1211(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_331
	dec	sp
	dec	sp
	ld	-1274(ix), l
	ld	-1273(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1276(ix), l
	ld	-1275(ix), h
	ld	l, -1276(ix)
	ld	h, -1275(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5363
	ld	hl, #0
	jp	__cmp_e_12497
__cmp_t_5363:
	ld	hl, #1
__cmp_e_12497:
	dec	sp
	dec	sp
	ld	-1278(ix), l
	ld	-1277(ix), h
	ld	l, -1278(ix)
	ld	h, -1277(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10253
	ld	hl, #0
	jp	__cmp_e_94892
__cmp_t_10253:
	ld	hl, #1
__cmp_e_94892:
	dec	sp
	dec	sp
	ld	-1280(ix), l
	ld	-1279(ix), h
	ld	l, -1280(ix)
	ld	h, -1279(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L335
	jp	__xcc_L336
__xcc_L336:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1282(ix), l
	ld	-1281(ix), h
	ld	l, -1282(ix)
	ld	h, -1281(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1284(ix), l
	ld	-1283(ix), h
	ld	l, -1284(ix)
	ld	h, -1283(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47686
	ld	hl, #0
	jp	__cmp_e_19125
__cmp_t_47686:
	ld	hl, #1
__cmp_e_19125:
	dec	sp
	dec	sp
	ld	-1286(ix), l
	ld	-1285(ix), h
	ld	l, -1286(ix)
	ld	h, -1285(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_61152
	ld	hl, #0
	jp	__cmp_e_13996
__cmp_t_61152:
	ld	hl, #1
__cmp_e_13996:
	dec	sp
	dec	sp
	ld	-1288(ix), l
	ld	-1287(ix), h
	jp	__xcc_L337
__xcc_L335:
	ld	hl, #1
	ld	-1288(ix), l
	ld	-1287(ix), h
__xcc_L337:
	ld	l, -1288(ix)
	ld	h, -1287(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L332
	jp	__xcc_L333
__xcc_L332:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1290(ix), l
	ld	-1289(ix), h
	ld	l, -1290(ix)
	ld	h, -1289(ix)
	dec	sp
	dec	sp
	ld	-1292(ix), l
	ld	-1291(ix), h
	jp	__xcc_L334
__xcc_L333:
	ld	hl, #1
	ld	-1292(ix), l
	ld	-1291(ix), h
__xcc_L334:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1294(ix), l
	ld	-1293(ix), h
	.globl __mul16
	ld	l, -1294(ix)
	ld	h, -1293(ix)
	push	hl
	ld	l, -1292(ix)
	ld	h, -1291(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1296(ix), l
	ld	-1295(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1298(ix), l
	ld	-1297(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1302(ix), l
	ld	-1301(ix), h
	ld	l, -1298(ix)
	ld	h, -1297(ix)
	push	hl
	ld	l, -1302(ix)
	ld	h, -1301(ix)
	ld	b, l
	pop	hl
__shift_5975:
	ld	a, b
	or	a, a
	jp	z, __sdone_9188
	add	hl, hl
	djnz	__shift_5975
__sdone_9188:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1306(ix), l
	ld	-1305(ix), h
	ld	l, -1306(ix)
	ld	h, -1305(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49157
	ld	hl, #0
	jp	__cmp_e_3729
__cmp_t_49157:
	ld	hl, #1
__cmp_e_3729:
	dec	sp
	dec	sp
	ld	-1308(ix), l
	ld	-1307(ix), h
	ld	l, -1308(ix)
	ld	h, -1307(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95436
	ld	hl, #0
	jp	__cmp_e_32460
__cmp_t_95436:
	ld	hl, #1
__cmp_e_32460:
	dec	sp
	dec	sp
	ld	-1310(ix), l
	ld	-1309(ix), h
	ld	l, -1310(ix)
	ld	h, -1309(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L341
	jp	__xcc_L342
__xcc_L342:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1312(ix), l
	ld	-1311(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1316(ix), l
	ld	-1315(ix), h
	ld	l, -1312(ix)
	ld	h, -1311(ix)
	push	hl
	ld	l, -1316(ix)
	ld	h, -1315(ix)
	ld	b, l
	pop	hl
__shift_3414:
	ld	a, b
	or	a, a
	jp	z, __sdone_3921
	add	hl, hl
	djnz	__shift_3414
__sdone_3921:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1320(ix), l
	ld	-1319(ix), h
	ld	l, -1320(ix)
	ld	h, -1319(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1322(ix), l
	ld	-1321(ix), h
	ld	l, -1322(ix)
	ld	h, -1321(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_70460
	ld	hl, #0
	jp	__cmp_e_26304
__cmp_t_70460:
	ld	hl, #1
__cmp_e_26304:
	dec	sp
	dec	sp
	ld	-1324(ix), l
	ld	-1323(ix), h
	ld	l, -1324(ix)
	ld	h, -1323(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60028
	ld	hl, #0
	jp	__cmp_e_88027
__cmp_t_60028:
	ld	hl, #1
__cmp_e_88027:
	dec	sp
	dec	sp
	ld	-1326(ix), l
	ld	-1325(ix), h
	jp	__xcc_L343
__xcc_L341:
	ld	hl, #1
	ld	-1326(ix), l
	ld	-1325(ix), h
__xcc_L343:
	ld	l, -1326(ix)
	ld	h, -1325(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L338
	jp	__xcc_L339
__xcc_L338:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1328(ix), l
	ld	-1327(ix), h
	ld	l, -1328(ix)
	ld	h, -1327(ix)
	dec	sp
	dec	sp
	ld	-1330(ix), l
	ld	-1329(ix), h
	jp	__xcc_L340
__xcc_L339:
	ld	hl, #1
	ld	-1330(ix), l
	ld	-1329(ix), h
__xcc_L340:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-1332(ix), l
	ld	-1331(ix), h
	.globl __mul16
	ld	l, -1332(ix)
	ld	h, -1331(ix)
	push	hl
	ld	l, -1330(ix)
	ld	h, -1329(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1334(ix), l
	ld	-1333(ix), h
	ld	l, -1334(ix)
	ld	h, -1333(ix)
	push	hl
	ld	l, -1296(ix)
	ld	h, -1295(ix)
	push	hl
	ld	l, -1274(ix)
	ld	h, -1273(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L316:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L315
	jp	__xcc_L317
__xcc_L317:
__xcc_L344:
	ld	hl, #__str_347
	dec	sp
	dec	sp
	ld	-1336(ix), l
	ld	-1335(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1338(ix), l
	ld	-1337(ix), h
	ld	l, -1338(ix)
	ld	h, -1337(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78050
	ld	hl, #0
	jp	__cmp_e_66748
__cmp_t_78050:
	ld	hl, #1
__cmp_e_66748:
	dec	sp
	dec	sp
	ld	-1340(ix), l
	ld	-1339(ix), h
	ld	l, -1340(ix)
	ld	h, -1339(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7556
	ld	hl, #0
	jp	__cmp_e_8902
__cmp_t_7556:
	ld	hl, #1
__cmp_e_8902:
	dec	sp
	dec	sp
	ld	-1342(ix), l
	ld	-1341(ix), h
	ld	l, -1342(ix)
	ld	h, -1341(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L351
	jp	__xcc_L352
__xcc_L352:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1344(ix), l
	ld	-1343(ix), h
	ld	l, -1344(ix)
	ld	h, -1343(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1346(ix), l
	ld	-1345(ix), h
	ld	l, -1346(ix)
	ld	h, -1345(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4794
	ld	hl, #0
	jp	__cmp_e_97697
__cmp_t_4794:
	ld	hl, #1
__cmp_e_97697:
	dec	sp
	dec	sp
	ld	-1348(ix), l
	ld	-1347(ix), h
	ld	l, -1348(ix)
	ld	h, -1347(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58699
	ld	hl, #0
	jp	__cmp_e_71043
__cmp_t_58699:
	ld	hl, #1
__cmp_e_71043:
	dec	sp
	dec	sp
	ld	-1350(ix), l
	ld	-1349(ix), h
	jp	__xcc_L353
__xcc_L351:
	ld	hl, #1
	ld	-1350(ix), l
	ld	-1349(ix), h
__xcc_L353:
	ld	l, -1350(ix)
	ld	h, -1349(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L348
	jp	__xcc_L349
__xcc_L348:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1352(ix), l
	ld	-1351(ix), h
	ld	l, -1352(ix)
	ld	h, -1351(ix)
	dec	sp
	dec	sp
	ld	-1354(ix), l
	ld	-1353(ix), h
	jp	__xcc_L350
__xcc_L349:
	ld	hl, #1
	ld	-1354(ix), l
	ld	-1353(ix), h
__xcc_L350:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1356(ix), l
	ld	-1355(ix), h
	.globl __mul16
	ld	l, -1356(ix)
	ld	h, -1355(ix)
	push	hl
	ld	l, -1354(ix)
	ld	h, -1353(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1358(ix), l
	ld	-1357(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1360(ix), l
	ld	-1359(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1368(ix), l
	ld	-1367(ix), h
	ld	l, -1360(ix)
	ld	h, -1359(ix)
	push	hl
	ld	l, -1368(ix)
	ld	h, -1367(ix)
	ld	b, l
	pop	hl
__shift_1039:
	ld	a, b
	or	a, a
	jp	z, __sdone_2002
	add	hl, hl
	djnz	__shift_1039
__sdone_2002:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1376(ix), l
	ld	-1375(ix), h
	ld	l, -1376(ix)
	ld	h, -1375(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90428
	ld	hl, #0
	jp	__cmp_e_6403
__cmp_t_90428:
	ld	hl, #1
__cmp_e_6403:
	dec	sp
	dec	sp
	ld	-1378(ix), l
	ld	-1377(ix), h
	ld	l, -1378(ix)
	ld	h, -1377(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44500
	ld	hl, #0
	jp	__cmp_e_681
__cmp_t_44500:
	ld	hl, #1
__cmp_e_681:
	dec	sp
	dec	sp
	ld	-1380(ix), l
	ld	-1379(ix), h
	ld	l, -1380(ix)
	ld	h, -1379(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L357
	jp	__xcc_L358
__xcc_L358:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1382(ix), l
	ld	-1381(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1390(ix), l
	ld	-1389(ix), h
	ld	l, -1382(ix)
	ld	h, -1381(ix)
	push	hl
	ld	l, -1390(ix)
	ld	h, -1389(ix)
	ld	b, l
	pop	hl
__shift_7647:
	ld	a, b
	or	a, a
	jp	z, __sdone_8538
	add	hl, hl
	djnz	__shift_7647
__sdone_8538:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1398(ix), l
	ld	-1397(ix), h
	ld	l, -1398(ix)
	ld	h, -1397(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1400(ix), l
	ld	-1399(ix), h
	ld	l, -1400(ix)
	ld	h, -1399(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_36159
	ld	hl, #0
	jp	__cmp_e_95151
__cmp_t_36159:
	ld	hl, #1
__cmp_e_95151:
	dec	sp
	dec	sp
	ld	-1402(ix), l
	ld	-1401(ix), h
	ld	l, -1402(ix)
	ld	h, -1401(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22535
	ld	hl, #0
	jp	__cmp_e_82134
__cmp_t_22535:
	ld	hl, #1
__cmp_e_82134:
	dec	sp
	dec	sp
	ld	-1404(ix), l
	ld	-1403(ix), h
	jp	__xcc_L359
__xcc_L357:
	ld	hl, #1
	ld	-1404(ix), l
	ld	-1403(ix), h
__xcc_L359:
	ld	l, -1404(ix)
	ld	h, -1403(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L354
	jp	__xcc_L355
__xcc_L354:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1406(ix), l
	ld	-1405(ix), h
	ld	l, -1406(ix)
	ld	h, -1405(ix)
	dec	sp
	dec	sp
	ld	-1408(ix), l
	ld	-1407(ix), h
	jp	__xcc_L356
__xcc_L355:
	ld	hl, #1
	ld	-1408(ix), l
	ld	-1407(ix), h
__xcc_L356:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-1410(ix), l
	ld	-1409(ix), h
	.globl __mul16
	ld	l, -1410(ix)
	ld	h, -1409(ix)
	push	hl
	ld	l, -1408(ix)
	ld	h, -1407(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1412(ix), l
	ld	-1411(ix), h
	ld	l, -1412(ix)
	ld	h, -1411(ix)
	push	hl
	ld	l, -1358(ix)
	ld	h, -1357(ix)
	push	hl
	ld	l, -1336(ix)
	ld	h, -1335(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_360
	dec	sp
	dec	sp
	ld	-1414(ix), l
	ld	-1413(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1416(ix), l
	ld	-1415(ix), h
	ld	l, -1416(ix)
	ld	h, -1415(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4339
	ld	hl, #0
	jp	__cmp_e_71692
__cmp_t_4339:
	ld	hl, #1
__cmp_e_71692:
	dec	sp
	dec	sp
	ld	-1418(ix), l
	ld	-1417(ix), h
	ld	l, -1418(ix)
	ld	h, -1417(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2215
	ld	hl, #0
	jp	__cmp_e_16127
__cmp_t_2215:
	ld	hl, #1
__cmp_e_16127:
	dec	sp
	dec	sp
	ld	-1420(ix), l
	ld	-1419(ix), h
	ld	l, -1420(ix)
	ld	h, -1419(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L364
	jp	__xcc_L365
__xcc_L365:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1422(ix), l
	ld	-1421(ix), h
	ld	l, -1422(ix)
	ld	h, -1421(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1424(ix), l
	ld	-1423(ix), h
	ld	l, -1424(ix)
	ld	h, -1423(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20504
	ld	hl, #0
	jp	__cmp_e_55629
__cmp_t_20504:
	ld	hl, #1
__cmp_e_55629:
	dec	sp
	dec	sp
	ld	-1426(ix), l
	ld	-1425(ix), h
	ld	l, -1426(ix)
	ld	h, -1425(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60049
	ld	hl, #0
	jp	__cmp_e_90964
__cmp_t_60049:
	ld	hl, #1
__cmp_e_90964:
	dec	sp
	dec	sp
	ld	-1428(ix), l
	ld	-1427(ix), h
	jp	__xcc_L366
__xcc_L364:
	ld	hl, #1
	ld	-1428(ix), l
	ld	-1427(ix), h
__xcc_L366:
	ld	l, -1428(ix)
	ld	h, -1427(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L361
	jp	__xcc_L362
__xcc_L361:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1430(ix), l
	ld	-1429(ix), h
	ld	l, -1430(ix)
	ld	h, -1429(ix)
	dec	sp
	dec	sp
	ld	-1432(ix), l
	ld	-1431(ix), h
	jp	__xcc_L363
__xcc_L362:
	ld	hl, #1
	ld	-1432(ix), l
	ld	-1431(ix), h
__xcc_L363:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1434(ix), l
	ld	-1433(ix), h
	.globl __mul16
	ld	l, -1434(ix)
	ld	h, -1433(ix)
	push	hl
	ld	l, -1432(ix)
	ld	h, -1431(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1436(ix), l
	ld	-1435(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1438(ix), l
	ld	-1437(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1446(ix), l
	ld	-1445(ix), h
	ld	l, -1438(ix)
	ld	h, -1437(ix)
	push	hl
	ld	l, -1446(ix)
	ld	h, -1445(ix)
	ld	b, l
	pop	hl
__shift_8285:
	ld	a, b
	or	a, a
	jp	z, __sdone_6429
	add	hl, hl
	djnz	__shift_8285
__sdone_6429:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1454(ix), l
	ld	-1453(ix), h
	ld	l, -1454(ix)
	ld	h, -1453(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_95343
	ld	hl, #0
	jp	__cmp_e_76335
__cmp_t_95343:
	ld	hl, #1
__cmp_e_76335:
	dec	sp
	dec	sp
	ld	-1456(ix), l
	ld	-1455(ix), h
	ld	l, -1456(ix)
	ld	h, -1455(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3177
	ld	hl, #0
	jp	__cmp_e_2900
__cmp_t_3177:
	ld	hl, #1
__cmp_e_2900:
	dec	sp
	dec	sp
	ld	-1458(ix), l
	ld	-1457(ix), h
	ld	l, -1458(ix)
	ld	h, -1457(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L370
	jp	__xcc_L371
__xcc_L371:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1460(ix), l
	ld	-1459(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1468(ix), l
	ld	-1467(ix), h
	ld	l, -1460(ix)
	ld	h, -1459(ix)
	push	hl
	ld	l, -1468(ix)
	ld	h, -1467(ix)
	ld	b, l
	pop	hl
__shift_5238:
	ld	a, b
	or	a, a
	jp	z, __sdone_7971
	add	hl, hl
	djnz	__shift_5238
__sdone_7971:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1476(ix), l
	ld	-1475(ix), h
	ld	l, -1476(ix)
	ld	h, -1475(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1478(ix), l
	ld	-1477(ix), h
	ld	l, -1478(ix)
	ld	h, -1477(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16949
	ld	hl, #0
	jp	__cmp_e_60289
__cmp_t_16949:
	ld	hl, #1
__cmp_e_60289:
	dec	sp
	dec	sp
	ld	-1480(ix), l
	ld	-1479(ix), h
	ld	l, -1480(ix)
	ld	h, -1479(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95367
	ld	hl, #0
	jp	__cmp_e_17988
__cmp_t_95367:
	ld	hl, #1
__cmp_e_17988:
	dec	sp
	dec	sp
	ld	-1482(ix), l
	ld	-1481(ix), h
	jp	__xcc_L372
__xcc_L370:
	ld	hl, #1
	ld	-1482(ix), l
	ld	-1481(ix), h
__xcc_L372:
	ld	l, -1482(ix)
	ld	h, -1481(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L367
	jp	__xcc_L368
__xcc_L367:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1484(ix), l
	ld	-1483(ix), h
	ld	l, -1484(ix)
	ld	h, -1483(ix)
	dec	sp
	dec	sp
	ld	-1486(ix), l
	ld	-1485(ix), h
	jp	__xcc_L369
__xcc_L368:
	ld	hl, #1
	ld	-1486(ix), l
	ld	-1485(ix), h
__xcc_L369:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-1488(ix), l
	ld	-1487(ix), h
	.globl __mul16
	ld	l, -1488(ix)
	ld	h, -1487(ix)
	push	hl
	ld	l, -1486(ix)
	ld	h, -1485(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1490(ix), l
	ld	-1489(ix), h
	ld	l, -1490(ix)
	ld	h, -1489(ix)
	push	hl
	ld	l, -1436(ix)
	ld	h, -1435(ix)
	push	hl
	ld	l, -1414(ix)
	ld	h, -1413(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L345:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L344
	jp	__xcc_L346
__xcc_L346:
__xcc_L255:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L254
	jp	__xcc_L256
__xcc_L256:
__xcc_L373:
__xcc_L376:
	ld	hl, #__str_379
	dec	sp
	dec	sp
	ld	-1492(ix), l
	ld	-1491(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1494(ix), l
	ld	-1493(ix), h
	ld	l, -1494(ix)
	ld	h, -1493(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92292
	ld	hl, #0
	jp	__cmp_e_85795
__cmp_t_92292:
	ld	hl, #1
__cmp_e_85795:
	dec	sp
	dec	sp
	ld	-1496(ix), l
	ld	-1495(ix), h
	ld	l, -1496(ix)
	ld	h, -1495(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_40743
	ld	hl, #0
	jp	__cmp_e_53144
__cmp_t_40743:
	ld	hl, #1
__cmp_e_53144:
	dec	sp
	dec	sp
	ld	-1498(ix), l
	ld	-1497(ix), h
	ld	l, -1498(ix)
	ld	h, -1497(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L383
	jp	__xcc_L384
__xcc_L384:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1500(ix), l
	ld	-1499(ix), h
	ld	l, -1500(ix)
	ld	h, -1499(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1502(ix), l
	ld	-1501(ix), h
	ld	l, -1502(ix)
	ld	h, -1501(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_2829
	ld	hl, #0
	jp	__cmp_e_58390
__cmp_t_2829:
	ld	hl, #1
__cmp_e_58390:
	dec	sp
	dec	sp
	ld	-1504(ix), l
	ld	-1503(ix), h
	ld	l, -1504(ix)
	ld	h, -1503(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_61682
	ld	hl, #0
	jp	__cmp_e_55340
__cmp_t_61682:
	ld	hl, #1
__cmp_e_55340:
	dec	sp
	dec	sp
	ld	-1506(ix), l
	ld	-1505(ix), h
	jp	__xcc_L385
__xcc_L383:
	ld	hl, #1
	ld	-1506(ix), l
	ld	-1505(ix), h
__xcc_L385:
	ld	l, -1506(ix)
	ld	h, -1505(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L380
	jp	__xcc_L381
__xcc_L380:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1508(ix), l
	ld	-1507(ix), h
	ld	l, -1508(ix)
	ld	h, -1507(ix)
	dec	sp
	dec	sp
	ld	-1510(ix), l
	ld	-1509(ix), h
	jp	__xcc_L382
__xcc_L381:
	ld	hl, #1
	ld	-1510(ix), l
	ld	-1509(ix), h
__xcc_L382:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1512(ix), l
	ld	-1511(ix), h
	.globl __mul16
	ld	l, -1512(ix)
	ld	h, -1511(ix)
	push	hl
	ld	l, -1510(ix)
	ld	h, -1509(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1514(ix), l
	ld	-1513(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1516(ix), l
	ld	-1515(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1518(ix), l
	ld	-1517(ix), h
	ld	l, -1516(ix)
	ld	h, -1515(ix)
	push	hl
	ld	l, -1518(ix)
	ld	h, -1517(ix)
	ld	b, l
	pop	hl
__shift_3541:
	ld	a, b
	or	a, a
	jp	z, __sdone_569
	add	hl, hl
	djnz	__shift_3541
__sdone_569:
	dec	sp
	dec	sp
	ld	-1520(ix), l
	ld	-1519(ix), h
	ld	l, -1520(ix)
	ld	h, -1519(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_53826
	ld	hl, #0
	jp	__cmp_e_74232
__cmp_t_53826:
	ld	hl, #1
__cmp_e_74232:
	dec	sp
	dec	sp
	ld	-1522(ix), l
	ld	-1521(ix), h
	ld	l, -1522(ix)
	ld	h, -1521(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_72261
	ld	hl, #0
	jp	__cmp_e_56042
__cmp_t_72261:
	ld	hl, #1
__cmp_e_56042:
	dec	sp
	dec	sp
	ld	-1524(ix), l
	ld	-1523(ix), h
	ld	l, -1524(ix)
	ld	h, -1523(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L389
	jp	__xcc_L390
__xcc_L390:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1526(ix), l
	ld	-1525(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1528(ix), l
	ld	-1527(ix), h
	ld	l, -1526(ix)
	ld	h, -1525(ix)
	push	hl
	ld	l, -1528(ix)
	ld	h, -1527(ix)
	ld	b, l
	pop	hl
__shift_360:
	ld	a, b
	or	a, a
	jp	z, __sdone_9117
	add	hl, hl
	djnz	__shift_360
__sdone_9117:
	dec	sp
	dec	sp
	ld	-1530(ix), l
	ld	-1529(ix), h
	ld	l, -1530(ix)
	ld	h, -1529(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1532(ix), l
	ld	-1531(ix), h
	ld	l, -1532(ix)
	ld	h, -1531(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28023
	ld	hl, #0
	jp	__cmp_e_66761
__cmp_t_28023:
	ld	hl, #1
__cmp_e_66761:
	dec	sp
	dec	sp
	ld	-1534(ix), l
	ld	-1533(ix), h
	ld	l, -1534(ix)
	ld	h, -1533(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81
	ld	hl, #0
	jp	__cmp_e_26309
__cmp_t_81:
	ld	hl, #1
__cmp_e_26309:
	dec	sp
	dec	sp
	ld	-1536(ix), l
	ld	-1535(ix), h
	jp	__xcc_L391
__xcc_L389:
	ld	hl, #1
	ld	-1536(ix), l
	ld	-1535(ix), h
__xcc_L391:
	ld	l, -1536(ix)
	ld	h, -1535(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L386
	jp	__xcc_L387
__xcc_L386:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1538(ix), l
	ld	-1537(ix), h
	ld	l, -1538(ix)
	ld	h, -1537(ix)
	dec	sp
	dec	sp
	ld	-1540(ix), l
	ld	-1539(ix), h
	jp	__xcc_L388
__xcc_L387:
	ld	hl, #1
	ld	-1540(ix), l
	ld	-1539(ix), h
__xcc_L388:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1542(ix), l
	ld	-1541(ix), h
	.globl __mul16
	ld	l, -1542(ix)
	ld	h, -1541(ix)
	push	hl
	ld	l, -1540(ix)
	ld	h, -1539(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1544(ix), l
	ld	-1543(ix), h
	ld	l, -1544(ix)
	ld	h, -1543(ix)
	push	hl
	ld	l, -1514(ix)
	ld	h, -1513(ix)
	push	hl
	ld	l, -1492(ix)
	ld	h, -1491(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_392
	dec	sp
	dec	sp
	ld	-1546(ix), l
	ld	-1545(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1548(ix), l
	ld	-1547(ix), h
	ld	l, -1548(ix)
	ld	h, -1547(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3190
	ld	hl, #0
	jp	__cmp_e_95425
__cmp_t_3190:
	ld	hl, #1
__cmp_e_95425:
	dec	sp
	dec	sp
	ld	-1550(ix), l
	ld	-1549(ix), h
	ld	l, -1550(ix)
	ld	h, -1549(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_18996
	ld	hl, #0
	jp	__cmp_e_6367
__cmp_t_18996:
	ld	hl, #1
__cmp_e_6367:
	dec	sp
	dec	sp
	ld	-1552(ix), l
	ld	-1551(ix), h
	ld	l, -1552(ix)
	ld	h, -1551(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L396
	jp	__xcc_L397
__xcc_L397:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1554(ix), l
	ld	-1553(ix), h
	ld	l, -1554(ix)
	ld	h, -1553(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1556(ix), l
	ld	-1555(ix), h
	ld	l, -1556(ix)
	ld	h, -1555(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_14677
	ld	hl, #0
	jp	__cmp_e_4234
__cmp_t_14677:
	ld	hl, #1
__cmp_e_4234:
	dec	sp
	dec	sp
	ld	-1558(ix), l
	ld	-1557(ix), h
	ld	l, -1558(ix)
	ld	h, -1557(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30690
	ld	hl, #0
	jp	__cmp_e_31626
__cmp_t_30690:
	ld	hl, #1
__cmp_e_31626:
	dec	sp
	dec	sp
	ld	-1560(ix), l
	ld	-1559(ix), h
	jp	__xcc_L398
__xcc_L396:
	ld	hl, #1
	ld	-1560(ix), l
	ld	-1559(ix), h
__xcc_L398:
	ld	l, -1560(ix)
	ld	h, -1559(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L393
	jp	__xcc_L394
__xcc_L393:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1562(ix), l
	ld	-1561(ix), h
	ld	l, -1562(ix)
	ld	h, -1561(ix)
	dec	sp
	dec	sp
	ld	-1564(ix), l
	ld	-1563(ix), h
	jp	__xcc_L395
__xcc_L394:
	ld	hl, #1
	ld	-1564(ix), l
	ld	-1563(ix), h
__xcc_L395:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1566(ix), l
	ld	-1565(ix), h
	.globl __mul16
	ld	l, -1566(ix)
	ld	h, -1565(ix)
	push	hl
	ld	l, -1564(ix)
	ld	h, -1563(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1568(ix), l
	ld	-1567(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1570(ix), l
	ld	-1569(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1572(ix), l
	ld	-1571(ix), h
	ld	l, -1570(ix)
	ld	h, -1569(ix)
	push	hl
	ld	l, -1572(ix)
	ld	h, -1571(ix)
	ld	b, l
	pop	hl
__shift_4524:
	ld	a, b
	or	a, a
	jp	z, __sdone_6057
	add	hl, hl
	djnz	__shift_4524
__sdone_6057:
	dec	sp
	dec	sp
	ld	-1574(ix), l
	ld	-1573(ix), h
	ld	l, -1574(ix)
	ld	h, -1573(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49614
	ld	hl, #0
	jp	__cmp_e_73168
__cmp_t_49614:
	ld	hl, #1
__cmp_e_73168:
	dec	sp
	dec	sp
	ld	-1576(ix), l
	ld	-1575(ix), h
	ld	l, -1576(ix)
	ld	h, -1575(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_28205
	ld	hl, #0
	jp	__cmp_e_90358
__cmp_t_28205:
	ld	hl, #1
__cmp_e_90358:
	dec	sp
	dec	sp
	ld	-1578(ix), l
	ld	-1577(ix), h
	ld	l, -1578(ix)
	ld	h, -1577(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L402
	jp	__xcc_L403
__xcc_L403:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1580(ix), l
	ld	-1579(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1582(ix), l
	ld	-1581(ix), h
	ld	l, -1580(ix)
	ld	h, -1579(ix)
	push	hl
	ld	l, -1582(ix)
	ld	h, -1581(ix)
	ld	b, l
	pop	hl
__shift_6312:
	ld	a, b
	or	a, a
	jp	z, __sdone_7386
	add	hl, hl
	djnz	__shift_6312
__sdone_7386:
	dec	sp
	dec	sp
	ld	-1584(ix), l
	ld	-1583(ix), h
	ld	l, -1584(ix)
	ld	h, -1583(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1586(ix), l
	ld	-1585(ix), h
	ld	l, -1586(ix)
	ld	h, -1585(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65100
	ld	hl, #0
	jp	__cmp_e_4346
__cmp_t_65100:
	ld	hl, #1
__cmp_e_4346:
	dec	sp
	dec	sp
	ld	-1588(ix), l
	ld	-1587(ix), h
	ld	l, -1588(ix)
	ld	h, -1587(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2726
	ld	hl, #0
	jp	__cmp_e_34994
__cmp_t_2726:
	ld	hl, #1
__cmp_e_34994:
	dec	sp
	dec	sp
	ld	-1590(ix), l
	ld	-1589(ix), h
	jp	__xcc_L404
__xcc_L402:
	ld	hl, #1
	ld	-1590(ix), l
	ld	-1589(ix), h
__xcc_L404:
	ld	l, -1590(ix)
	ld	h, -1589(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L399
	jp	__xcc_L400
__xcc_L399:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1592(ix), l
	ld	-1591(ix), h
	ld	l, -1592(ix)
	ld	h, -1591(ix)
	dec	sp
	dec	sp
	ld	-1594(ix), l
	ld	-1593(ix), h
	jp	__xcc_L401
__xcc_L400:
	ld	hl, #1
	ld	-1594(ix), l
	ld	-1593(ix), h
__xcc_L401:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1596(ix), l
	ld	-1595(ix), h
	.globl __mul16
	ld	l, -1596(ix)
	ld	h, -1595(ix)
	push	hl
	ld	l, -1594(ix)
	ld	h, -1593(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1598(ix), l
	ld	-1597(ix), h
	ld	l, -1598(ix)
	ld	h, -1597(ix)
	push	hl
	ld	l, -1568(ix)
	ld	h, -1567(ix)
	push	hl
	ld	l, -1546(ix)
	ld	h, -1545(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L377:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L376
	jp	__xcc_L378
__xcc_L378:
__xcc_L405:
	ld	hl, #__str_408
	dec	sp
	dec	sp
	ld	-1600(ix), l
	ld	-1599(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1602(ix), l
	ld	-1601(ix), h
	ld	l, -1602(ix)
	ld	h, -1601(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4916
	ld	hl, #0
	jp	__cmp_e_56552
__cmp_t_4916:
	ld	hl, #1
__cmp_e_56552:
	dec	sp
	dec	sp
	ld	-1604(ix), l
	ld	-1603(ix), h
	ld	l, -1604(ix)
	ld	h, -1603(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25578
	ld	hl, #0
	jp	__cmp_e_93529
__cmp_t_25578:
	ld	hl, #1
__cmp_e_93529:
	dec	sp
	dec	sp
	ld	-1606(ix), l
	ld	-1605(ix), h
	ld	l, -1606(ix)
	ld	h, -1605(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L412
	jp	__xcc_L413
__xcc_L413:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1608(ix), l
	ld	-1607(ix), h
	ld	l, -1608(ix)
	ld	h, -1607(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1610(ix), l
	ld	-1609(ix), h
	ld	l, -1610(ix)
	ld	h, -1609(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28946
	ld	hl, #0
	jp	__cmp_e_32290
__cmp_t_28946:
	ld	hl, #1
__cmp_e_32290:
	dec	sp
	dec	sp
	ld	-1612(ix), l
	ld	-1611(ix), h
	ld	l, -1612(ix)
	ld	h, -1611(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2647
	ld	hl, #0
	jp	__cmp_e_56970
__cmp_t_2647:
	ld	hl, #1
__cmp_e_56970:
	dec	sp
	dec	sp
	ld	-1614(ix), l
	ld	-1613(ix), h
	jp	__xcc_L414
__xcc_L412:
	ld	hl, #1
	ld	-1614(ix), l
	ld	-1613(ix), h
__xcc_L414:
	ld	l, -1614(ix)
	ld	h, -1613(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L409
	jp	__xcc_L410
__xcc_L409:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1616(ix), l
	ld	-1615(ix), h
	ld	l, -1616(ix)
	ld	h, -1615(ix)
	dec	sp
	dec	sp
	ld	-1618(ix), l
	ld	-1617(ix), h
	jp	__xcc_L411
__xcc_L410:
	ld	hl, #1
	ld	-1618(ix), l
	ld	-1617(ix), h
__xcc_L411:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1620(ix), l
	ld	-1619(ix), h
	.globl __mul16
	ld	l, -1620(ix)
	ld	h, -1619(ix)
	push	hl
	ld	l, -1618(ix)
	ld	h, -1617(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1622(ix), l
	ld	-1621(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1624(ix), l
	ld	-1623(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1626(ix), l
	ld	-1625(ix), h
	ld	l, -1624(ix)
	ld	h, -1623(ix)
	push	hl
	ld	l, -1626(ix)
	ld	h, -1625(ix)
	ld	b, l
	pop	hl
__shift_9051:
	ld	a, b
	or	a, a
	jp	z, __sdone_9080
	add	hl, hl
	djnz	__shift_9051
__sdone_9080:
	dec	sp
	dec	sp
	ld	-1628(ix), l
	ld	-1627(ix), h
	ld	l, -1628(ix)
	ld	h, -1627(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99631
	ld	hl, #0
	jp	__cmp_e_18593
__cmp_t_99631:
	ld	hl, #1
__cmp_e_18593:
	dec	sp
	dec	sp
	ld	-1630(ix), l
	ld	-1629(ix), h
	ld	l, -1630(ix)
	ld	h, -1629(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30857
	ld	hl, #0
	jp	__cmp_e_18627
__cmp_t_30857:
	ld	hl, #1
__cmp_e_18627:
	dec	sp
	dec	sp
	ld	-1632(ix), l
	ld	-1631(ix), h
	ld	l, -1632(ix)
	ld	h, -1631(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L418
	jp	__xcc_L419
__xcc_L419:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1634(ix), l
	ld	-1633(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1636(ix), l
	ld	-1635(ix), h
	ld	l, -1634(ix)
	ld	h, -1633(ix)
	push	hl
	ld	l, -1636(ix)
	ld	h, -1635(ix)
	ld	b, l
	pop	hl
__shift_1312:
	ld	a, b
	or	a, a
	jp	z, __sdone_1886
	add	hl, hl
	djnz	__shift_1312
__sdone_1886:
	dec	sp
	dec	sp
	ld	-1638(ix), l
	ld	-1637(ix), h
	ld	l, -1638(ix)
	ld	h, -1637(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1640(ix), l
	ld	-1639(ix), h
	ld	l, -1640(ix)
	ld	h, -1639(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39214
	ld	hl, #0
	jp	__cmp_e_88355
__cmp_t_39214:
	ld	hl, #1
__cmp_e_88355:
	dec	sp
	dec	sp
	ld	-1642(ix), l
	ld	-1641(ix), h
	ld	l, -1642(ix)
	ld	h, -1641(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93512
	ld	hl, #0
	jp	__cmp_e_20090
__cmp_t_93512:
	ld	hl, #1
__cmp_e_20090:
	dec	sp
	dec	sp
	ld	-1644(ix), l
	ld	-1643(ix), h
	jp	__xcc_L420
__xcc_L418:
	ld	hl, #1
	ld	-1644(ix), l
	ld	-1643(ix), h
__xcc_L420:
	ld	l, -1644(ix)
	ld	h, -1643(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L415
	jp	__xcc_L416
__xcc_L415:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1646(ix), l
	ld	-1645(ix), h
	ld	l, -1646(ix)
	ld	h, -1645(ix)
	dec	sp
	dec	sp
	ld	-1648(ix), l
	ld	-1647(ix), h
	jp	__xcc_L417
__xcc_L416:
	ld	hl, #1
	ld	-1648(ix), l
	ld	-1647(ix), h
__xcc_L417:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1650(ix), l
	ld	-1649(ix), h
	.globl __mul16
	ld	l, -1650(ix)
	ld	h, -1649(ix)
	push	hl
	ld	l, -1648(ix)
	ld	h, -1647(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1652(ix), l
	ld	-1651(ix), h
	ld	l, -1652(ix)
	ld	h, -1651(ix)
	push	hl
	ld	l, -1622(ix)
	ld	h, -1621(ix)
	push	hl
	ld	l, -1600(ix)
	ld	h, -1599(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_421
	dec	sp
	dec	sp
	ld	-1654(ix), l
	ld	-1653(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1656(ix), l
	ld	-1655(ix), h
	ld	l, -1656(ix)
	ld	h, -1655(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_14412
	ld	hl, #0
	jp	__cmp_e_59479
__cmp_t_14412:
	ld	hl, #1
__cmp_e_59479:
	dec	sp
	dec	sp
	ld	-1658(ix), l
	ld	-1657(ix), h
	ld	l, -1658(ix)
	ld	h, -1657(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_9610
	ld	hl, #0
	jp	__cmp_e_58969
__cmp_t_9610:
	ld	hl, #1
__cmp_e_58969:
	dec	sp
	dec	sp
	ld	-1660(ix), l
	ld	-1659(ix), h
	ld	l, -1660(ix)
	ld	h, -1659(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L425
	jp	__xcc_L426
__xcc_L426:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1662(ix), l
	ld	-1661(ix), h
	ld	l, -1662(ix)
	ld	h, -1661(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1664(ix), l
	ld	-1663(ix), h
	ld	l, -1664(ix)
	ld	h, -1663(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_66189
	ld	hl, #0
	jp	__cmp_e_52274
__cmp_t_66189:
	ld	hl, #1
__cmp_e_52274:
	dec	sp
	dec	sp
	ld	-1666(ix), l
	ld	-1665(ix), h
	ld	l, -1666(ix)
	ld	h, -1665(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6355
	ld	hl, #0
	jp	__cmp_e_47641
__cmp_t_6355:
	ld	hl, #1
__cmp_e_47641:
	dec	sp
	dec	sp
	ld	-1668(ix), l
	ld	-1667(ix), h
	jp	__xcc_L427
__xcc_L425:
	ld	hl, #1
	ld	-1668(ix), l
	ld	-1667(ix), h
__xcc_L427:
	ld	l, -1668(ix)
	ld	h, -1667(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L422
	jp	__xcc_L423
__xcc_L422:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1670(ix), l
	ld	-1669(ix), h
	ld	l, -1670(ix)
	ld	h, -1669(ix)
	dec	sp
	dec	sp
	ld	-1672(ix), l
	ld	-1671(ix), h
	jp	__xcc_L424
__xcc_L423:
	ld	hl, #1
	ld	-1672(ix), l
	ld	-1671(ix), h
__xcc_L424:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1674(ix), l
	ld	-1673(ix), h
	.globl __mul16
	ld	l, -1674(ix)
	ld	h, -1673(ix)
	push	hl
	ld	l, -1672(ix)
	ld	h, -1671(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1676(ix), l
	ld	-1675(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1678(ix), l
	ld	-1677(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1680(ix), l
	ld	-1679(ix), h
	ld	l, -1678(ix)
	ld	h, -1677(ix)
	push	hl
	ld	l, -1680(ix)
	ld	h, -1679(ix)
	ld	b, l
	pop	hl
__shift_6620:
	ld	a, b
	or	a, a
	jp	z, __sdone_5433
	add	hl, hl
	djnz	__shift_6620
__sdone_5433:
	dec	sp
	dec	sp
	ld	-1682(ix), l
	ld	-1681(ix), h
	ld	l, -1682(ix)
	ld	h, -1681(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98987
	ld	hl, #0
	jp	__cmp_e_77888
__cmp_t_98987:
	ld	hl, #1
__cmp_e_77888:
	dec	sp
	dec	sp
	ld	-1684(ix), l
	ld	-1683(ix), h
	ld	l, -1684(ix)
	ld	h, -1683(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98338
	ld	hl, #0
	jp	__cmp_e_24566
__cmp_t_98338:
	ld	hl, #1
__cmp_e_24566:
	dec	sp
	dec	sp
	ld	-1686(ix), l
	ld	-1685(ix), h
	ld	l, -1686(ix)
	ld	h, -1685(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L431
	jp	__xcc_L432
__xcc_L432:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1688(ix), l
	ld	-1687(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1690(ix), l
	ld	-1689(ix), h
	ld	l, -1688(ix)
	ld	h, -1687(ix)
	push	hl
	ld	l, -1690(ix)
	ld	h, -1689(ix)
	ld	b, l
	pop	hl
__shift_7770:
	ld	a, b
	or	a, a
	jp	z, __sdone_7284
	add	hl, hl
	djnz	__shift_7770
__sdone_7284:
	dec	sp
	dec	sp
	ld	-1692(ix), l
	ld	-1691(ix), h
	ld	l, -1692(ix)
	ld	h, -1691(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1694(ix), l
	ld	-1693(ix), h
	ld	l, -1694(ix)
	ld	h, -1693(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_56856
	ld	hl, #0
	jp	__cmp_e_90417
__cmp_t_56856:
	ld	hl, #1
__cmp_e_90417:
	dec	sp
	dec	sp
	ld	-1696(ix), l
	ld	-1695(ix), h
	ld	l, -1696(ix)
	ld	h, -1695(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_606
	ld	hl, #0
	jp	__cmp_e_72260
__cmp_t_606:
	ld	hl, #1
__cmp_e_72260:
	dec	sp
	dec	sp
	ld	-1698(ix), l
	ld	-1697(ix), h
	jp	__xcc_L433
__xcc_L431:
	ld	hl, #1
	ld	-1698(ix), l
	ld	-1697(ix), h
__xcc_L433:
	ld	l, -1698(ix)
	ld	h, -1697(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L428
	jp	__xcc_L429
__xcc_L428:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1700(ix), l
	ld	-1699(ix), h
	ld	l, -1700(ix)
	ld	h, -1699(ix)
	dec	sp
	dec	sp
	ld	-1702(ix), l
	ld	-1701(ix), h
	jp	__xcc_L430
__xcc_L429:
	ld	hl, #1
	ld	-1702(ix), l
	ld	-1701(ix), h
__xcc_L430:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1704(ix), l
	ld	-1703(ix), h
	.globl __mul16
	ld	l, -1704(ix)
	ld	h, -1703(ix)
	push	hl
	ld	l, -1702(ix)
	ld	h, -1701(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1706(ix), l
	ld	-1705(ix), h
	ld	l, -1706(ix)
	ld	h, -1705(ix)
	push	hl
	ld	l, -1676(ix)
	ld	h, -1675(ix)
	push	hl
	ld	l, -1654(ix)
	ld	h, -1653(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L406:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L405
	jp	__xcc_L407
__xcc_L407:
__xcc_L434:
	ld	hl, #__str_437
	dec	sp
	dec	sp
	ld	-1708(ix), l
	ld	-1707(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1710(ix), l
	ld	-1709(ix), h
	ld	l, -1710(ix)
	ld	h, -1709(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_25849
	ld	hl, #0
	jp	__cmp_e_237
__cmp_t_25849:
	ld	hl, #1
__cmp_e_237:
	dec	sp
	dec	sp
	ld	-1712(ix), l
	ld	-1711(ix), h
	ld	l, -1712(ix)
	ld	h, -1711(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7205
	ld	hl, #0
	jp	__cmp_e_73059
__cmp_t_7205:
	ld	hl, #1
__cmp_e_73059:
	dec	sp
	dec	sp
	ld	-1714(ix), l
	ld	-1713(ix), h
	ld	l, -1714(ix)
	ld	h, -1713(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L441
	jp	__xcc_L442
__xcc_L442:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1716(ix), l
	ld	-1715(ix), h
	ld	l, -1716(ix)
	ld	h, -1715(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1718(ix), l
	ld	-1717(ix), h
	ld	l, -1718(ix)
	ld	h, -1717(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35217
	ld	hl, #0
	jp	__cmp_e_48518
__cmp_t_35217:
	ld	hl, #1
__cmp_e_48518:
	dec	sp
	dec	sp
	ld	-1720(ix), l
	ld	-1719(ix), h
	ld	l, -1720(ix)
	ld	h, -1719(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34945
	ld	hl, #0
	jp	__cmp_e_90783
__cmp_t_34945:
	ld	hl, #1
__cmp_e_90783:
	dec	sp
	dec	sp
	ld	-1722(ix), l
	ld	-1721(ix), h
	jp	__xcc_L443
__xcc_L441:
	ld	hl, #1
	ld	-1722(ix), l
	ld	-1721(ix), h
__xcc_L443:
	ld	l, -1722(ix)
	ld	h, -1721(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L438
	jp	__xcc_L439
__xcc_L438:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1724(ix), l
	ld	-1723(ix), h
	ld	l, -1724(ix)
	ld	h, -1723(ix)
	dec	sp
	dec	sp
	ld	-1726(ix), l
	ld	-1725(ix), h
	jp	__xcc_L440
__xcc_L439:
	ld	hl, #1
	ld	-1726(ix), l
	ld	-1725(ix), h
__xcc_L440:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1728(ix), l
	ld	-1727(ix), h
	.globl __mul16
	ld	l, -1728(ix)
	ld	h, -1727(ix)
	push	hl
	ld	l, -1726(ix)
	ld	h, -1725(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1730(ix), l
	ld	-1729(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1732(ix), l
	ld	-1731(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1736(ix), l
	ld	-1735(ix), h
	ld	l, -1732(ix)
	ld	h, -1731(ix)
	push	hl
	ld	l, -1736(ix)
	ld	h, -1735(ix)
	ld	b, l
	pop	hl
__shift_6873:
	ld	a, b
	or	a, a
	jp	z, __sdone_8458
	add	hl, hl
	djnz	__shift_6873
__sdone_8458:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1740(ix), l
	ld	-1739(ix), h
	ld	l, -1740(ix)
	ld	h, -1739(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_10873
	ld	hl, #0
	jp	__cmp_e_67637
__cmp_t_10873:
	ld	hl, #1
__cmp_e_67637:
	dec	sp
	dec	sp
	ld	-1742(ix), l
	ld	-1741(ix), h
	ld	l, -1742(ix)
	ld	h, -1741(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4289
	ld	hl, #0
	jp	__cmp_e_20483
__cmp_t_4289:
	ld	hl, #1
__cmp_e_20483:
	dec	sp
	dec	sp
	ld	-1744(ix), l
	ld	-1743(ix), h
	ld	l, -1744(ix)
	ld	h, -1743(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L447
	jp	__xcc_L448
__xcc_L448:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1746(ix), l
	ld	-1745(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1750(ix), l
	ld	-1749(ix), h
	ld	l, -1746(ix)
	ld	h, -1745(ix)
	push	hl
	ld	l, -1750(ix)
	ld	h, -1749(ix)
	ld	b, l
	pop	hl
__shift_6607:
	ld	a, b
	or	a, a
	jp	z, __sdone_478
	add	hl, hl
	djnz	__shift_6607
__sdone_478:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1754(ix), l
	ld	-1753(ix), h
	ld	l, -1754(ix)
	ld	h, -1753(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1756(ix), l
	ld	-1755(ix), h
	ld	l, -1756(ix)
	ld	h, -1755(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72757
	ld	hl, #0
	jp	__cmp_e_49314
__cmp_t_72757:
	ld	hl, #1
__cmp_e_49314:
	dec	sp
	dec	sp
	ld	-1758(ix), l
	ld	-1757(ix), h
	ld	l, -1758(ix)
	ld	h, -1757(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34471
	ld	hl, #0
	jp	__cmp_e_45729
__cmp_t_34471:
	ld	hl, #1
__cmp_e_45729:
	dec	sp
	dec	sp
	ld	-1760(ix), l
	ld	-1759(ix), h
	jp	__xcc_L449
__xcc_L447:
	ld	hl, #1
	ld	-1760(ix), l
	ld	-1759(ix), h
__xcc_L449:
	ld	l, -1760(ix)
	ld	h, -1759(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L444
	jp	__xcc_L445
__xcc_L444:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1762(ix), l
	ld	-1761(ix), h
	ld	l, -1762(ix)
	ld	h, -1761(ix)
	dec	sp
	dec	sp
	ld	-1764(ix), l
	ld	-1763(ix), h
	jp	__xcc_L446
__xcc_L445:
	ld	hl, #1
	ld	-1764(ix), l
	ld	-1763(ix), h
__xcc_L446:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-1766(ix), l
	ld	-1765(ix), h
	.globl __mul16
	ld	l, -1766(ix)
	ld	h, -1765(ix)
	push	hl
	ld	l, -1764(ix)
	ld	h, -1763(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1768(ix), l
	ld	-1767(ix), h
	ld	l, -1768(ix)
	ld	h, -1767(ix)
	push	hl
	ld	l, -1730(ix)
	ld	h, -1729(ix)
	push	hl
	ld	l, -1708(ix)
	ld	h, -1707(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_450
	dec	sp
	dec	sp
	ld	-1770(ix), l
	ld	-1769(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1772(ix), l
	ld	-1771(ix), h
	ld	l, -1772(ix)
	ld	h, -1771(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91100
	ld	hl, #0
	jp	__cmp_e_33459
__cmp_t_91100:
	ld	hl, #1
__cmp_e_33459:
	dec	sp
	dec	sp
	ld	-1774(ix), l
	ld	-1773(ix), h
	ld	l, -1774(ix)
	ld	h, -1773(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_23618
	ld	hl, #0
	jp	__cmp_e_89438
__cmp_t_23618:
	ld	hl, #1
__cmp_e_89438:
	dec	sp
	dec	sp
	ld	-1776(ix), l
	ld	-1775(ix), h
	ld	l, -1776(ix)
	ld	h, -1775(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L454
	jp	__xcc_L455
__xcc_L455:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1778(ix), l
	ld	-1777(ix), h
	ld	l, -1778(ix)
	ld	h, -1777(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1780(ix), l
	ld	-1779(ix), h
	ld	l, -1780(ix)
	ld	h, -1779(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_58025
	ld	hl, #0
	jp	__cmp_e_11388
__cmp_t_58025:
	ld	hl, #1
__cmp_e_11388:
	dec	sp
	dec	sp
	ld	-1782(ix), l
	ld	-1781(ix), h
	ld	l, -1782(ix)
	ld	h, -1781(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33074
	ld	hl, #0
	jp	__cmp_e_31233
__cmp_t_33074:
	ld	hl, #1
__cmp_e_31233:
	dec	sp
	dec	sp
	ld	-1784(ix), l
	ld	-1783(ix), h
	jp	__xcc_L456
__xcc_L454:
	ld	hl, #1
	ld	-1784(ix), l
	ld	-1783(ix), h
__xcc_L456:
	ld	l, -1784(ix)
	ld	h, -1783(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L451
	jp	__xcc_L452
__xcc_L451:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1786(ix), l
	ld	-1785(ix), h
	ld	l, -1786(ix)
	ld	h, -1785(ix)
	dec	sp
	dec	sp
	ld	-1788(ix), l
	ld	-1787(ix), h
	jp	__xcc_L453
__xcc_L452:
	ld	hl, #1
	ld	-1788(ix), l
	ld	-1787(ix), h
__xcc_L453:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1790(ix), l
	ld	-1789(ix), h
	.globl __mul16
	ld	l, -1790(ix)
	ld	h, -1789(ix)
	push	hl
	ld	l, -1788(ix)
	ld	h, -1787(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1792(ix), l
	ld	-1791(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1794(ix), l
	ld	-1793(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1798(ix), l
	ld	-1797(ix), h
	ld	l, -1794(ix)
	ld	h, -1793(ix)
	push	hl
	ld	l, -1798(ix)
	ld	h, -1797(ix)
	ld	b, l
	pop	hl
__shift_8157:
	ld	a, b
	or	a, a
	jp	z, __sdone_3681
	add	hl, hl
	djnz	__shift_8157
__sdone_3681:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1802(ix), l
	ld	-1801(ix), h
	ld	l, -1802(ix)
	ld	h, -1801(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3493
	ld	hl, #0
	jp	__cmp_e_60358
__cmp_t_3493:
	ld	hl, #1
__cmp_e_60358:
	dec	sp
	dec	sp
	ld	-1804(ix), l
	ld	-1803(ix), h
	ld	l, -1804(ix)
	ld	h, -1803(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50270
	ld	hl, #0
	jp	__cmp_e_10699
__cmp_t_50270:
	ld	hl, #1
__cmp_e_10699:
	dec	sp
	dec	sp
	ld	-1806(ix), l
	ld	-1805(ix), h
	ld	l, -1806(ix)
	ld	h, -1805(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L460
	jp	__xcc_L461
__xcc_L461:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1808(ix), l
	ld	-1807(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1812(ix), l
	ld	-1811(ix), h
	ld	l, -1808(ix)
	ld	h, -1807(ix)
	push	hl
	ld	l, -1812(ix)
	ld	h, -1811(ix)
	ld	b, l
	pop	hl
__shift_3417:
	ld	a, b
	or	a, a
	jp	z, __sdone_1839
	add	hl, hl
	djnz	__shift_3417
__sdone_1839:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1816(ix), l
	ld	-1815(ix), h
	ld	l, -1816(ix)
	ld	h, -1815(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1818(ix), l
	ld	-1817(ix), h
	ld	l, -1818(ix)
	ld	h, -1817(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_75569
	ld	hl, #0
	jp	__cmp_e_68363
__cmp_t_75569:
	ld	hl, #1
__cmp_e_68363:
	dec	sp
	dec	sp
	ld	-1820(ix), l
	ld	-1819(ix), h
	ld	l, -1820(ix)
	ld	h, -1819(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92622
	ld	hl, #0
	jp	__cmp_e_28794
__cmp_t_92622:
	ld	hl, #1
__cmp_e_28794:
	dec	sp
	dec	sp
	ld	-1822(ix), l
	ld	-1821(ix), h
	jp	__xcc_L462
__xcc_L460:
	ld	hl, #1
	ld	-1822(ix), l
	ld	-1821(ix), h
__xcc_L462:
	ld	l, -1822(ix)
	ld	h, -1821(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L457
	jp	__xcc_L458
__xcc_L457:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1824(ix), l
	ld	-1823(ix), h
	ld	l, -1824(ix)
	ld	h, -1823(ix)
	dec	sp
	dec	sp
	ld	-1826(ix), l
	ld	-1825(ix), h
	jp	__xcc_L459
__xcc_L458:
	ld	hl, #1
	ld	-1826(ix), l
	ld	-1825(ix), h
__xcc_L459:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-1828(ix), l
	ld	-1827(ix), h
	.globl __mul16
	ld	l, -1828(ix)
	ld	h, -1827(ix)
	push	hl
	ld	l, -1826(ix)
	ld	h, -1825(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1830(ix), l
	ld	-1829(ix), h
	ld	l, -1830(ix)
	ld	h, -1829(ix)
	push	hl
	ld	l, -1792(ix)
	ld	h, -1791(ix)
	push	hl
	ld	l, -1770(ix)
	ld	h, -1769(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L435:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L434
	jp	__xcc_L436
__xcc_L436:
__xcc_L463:
	ld	hl, #__str_466
	dec	sp
	dec	sp
	ld	-1832(ix), l
	ld	-1831(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1834(ix), l
	ld	-1833(ix), h
	ld	l, -1834(ix)
	ld	h, -1833(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13173
	ld	hl, #0
	jp	__cmp_e_19847
__cmp_t_13173:
	ld	hl, #1
__cmp_e_19847:
	dec	sp
	dec	sp
	ld	-1836(ix), l
	ld	-1835(ix), h
	ld	l, -1836(ix)
	ld	h, -1835(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96431
	ld	hl, #0
	jp	__cmp_e_17462
__cmp_t_96431:
	ld	hl, #1
__cmp_e_17462:
	dec	sp
	dec	sp
	ld	-1838(ix), l
	ld	-1837(ix), h
	ld	l, -1838(ix)
	ld	h, -1837(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L470
	jp	__xcc_L471
__xcc_L471:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1840(ix), l
	ld	-1839(ix), h
	ld	l, -1840(ix)
	ld	h, -1839(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1842(ix), l
	ld	-1841(ix), h
	ld	l, -1842(ix)
	ld	h, -1841(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_56682
	ld	hl, #0
	jp	__cmp_e_39390
__cmp_t_56682:
	ld	hl, #1
__cmp_e_39390:
	dec	sp
	dec	sp
	ld	-1844(ix), l
	ld	-1843(ix), h
	ld	l, -1844(ix)
	ld	h, -1843(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4292
	ld	hl, #0
	jp	__cmp_e_45791
__cmp_t_4292:
	ld	hl, #1
__cmp_e_45791:
	dec	sp
	dec	sp
	ld	-1846(ix), l
	ld	-1845(ix), h
	jp	__xcc_L472
__xcc_L470:
	ld	hl, #1
	ld	-1846(ix), l
	ld	-1845(ix), h
__xcc_L472:
	ld	l, -1846(ix)
	ld	h, -1845(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L467
	jp	__xcc_L468
__xcc_L467:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1848(ix), l
	ld	-1847(ix), h
	ld	l, -1848(ix)
	ld	h, -1847(ix)
	dec	sp
	dec	sp
	ld	-1850(ix), l
	ld	-1849(ix), h
	jp	__xcc_L469
__xcc_L468:
	ld	hl, #1
	ld	-1850(ix), l
	ld	-1849(ix), h
__xcc_L469:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1852(ix), l
	ld	-1851(ix), h
	.globl __mul16
	ld	l, -1852(ix)
	ld	h, -1851(ix)
	push	hl
	ld	l, -1850(ix)
	ld	h, -1849(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1854(ix), l
	ld	-1853(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1856(ix), l
	ld	-1855(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1864(ix), l
	ld	-1863(ix), h
	ld	l, -1856(ix)
	ld	h, -1855(ix)
	push	hl
	ld	l, -1864(ix)
	ld	h, -1863(ix)
	ld	b, l
	pop	hl
__shift_5057:
	ld	a, b
	or	a, a
	jp	z, __sdone_5115
	add	hl, hl
	djnz	__shift_5057
__sdone_5115:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1872(ix), l
	ld	-1871(ix), h
	ld	l, -1872(ix)
	ld	h, -1871(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91521
	ld	hl, #0
	jp	__cmp_e_96157
__cmp_t_91521:
	ld	hl, #1
__cmp_e_96157:
	dec	sp
	dec	sp
	ld	-1874(ix), l
	ld	-1873(ix), h
	ld	l, -1874(ix)
	ld	h, -1873(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88574
	ld	hl, #0
	jp	__cmp_e_31491
__cmp_t_88574:
	ld	hl, #1
__cmp_e_31491:
	dec	sp
	dec	sp
	ld	-1876(ix), l
	ld	-1875(ix), h
	ld	l, -1876(ix)
	ld	h, -1875(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L476
	jp	__xcc_L477
__xcc_L477:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1878(ix), l
	ld	-1877(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1886(ix), l
	ld	-1885(ix), h
	ld	l, -1878(ix)
	ld	h, -1877(ix)
	push	hl
	ld	l, -1886(ix)
	ld	h, -1885(ix)
	ld	b, l
	pop	hl
__shift_1947:
	ld	a, b
	or	a, a
	jp	z, __sdone_2951
	add	hl, hl
	djnz	__shift_1947
__sdone_2951:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1894(ix), l
	ld	-1893(ix), h
	ld	l, -1894(ix)
	ld	h, -1893(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1896(ix), l
	ld	-1895(ix), h
	ld	l, -1896(ix)
	ld	h, -1895(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59231
	ld	hl, #0
	jp	__cmp_e_35021
__cmp_t_59231:
	ld	hl, #1
__cmp_e_35021:
	dec	sp
	dec	sp
	ld	-1898(ix), l
	ld	-1897(ix), h
	ld	l, -1898(ix)
	ld	h, -1897(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10537
	ld	hl, #0
	jp	__cmp_e_93740
__cmp_t_10537:
	ld	hl, #1
__cmp_e_93740:
	dec	sp
	dec	sp
	ld	-1900(ix), l
	ld	-1899(ix), h
	jp	__xcc_L478
__xcc_L476:
	ld	hl, #1
	ld	-1900(ix), l
	ld	-1899(ix), h
__xcc_L478:
	ld	l, -1900(ix)
	ld	h, -1899(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L473
	jp	__xcc_L474
__xcc_L473:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1902(ix), l
	ld	-1901(ix), h
	ld	l, -1902(ix)
	ld	h, -1901(ix)
	dec	sp
	dec	sp
	ld	-1904(ix), l
	ld	-1903(ix), h
	jp	__xcc_L475
__xcc_L474:
	ld	hl, #1
	ld	-1904(ix), l
	ld	-1903(ix), h
__xcc_L475:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-1906(ix), l
	ld	-1905(ix), h
	.globl __mul16
	ld	l, -1906(ix)
	ld	h, -1905(ix)
	push	hl
	ld	l, -1904(ix)
	ld	h, -1903(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1908(ix), l
	ld	-1907(ix), h
	ld	l, -1908(ix)
	ld	h, -1907(ix)
	push	hl
	ld	l, -1854(ix)
	ld	h, -1853(ix)
	push	hl
	ld	l, -1832(ix)
	ld	h, -1831(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_479
	dec	sp
	dec	sp
	ld	-1910(ix), l
	ld	-1909(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1912(ix), l
	ld	-1911(ix), h
	ld	l, -1912(ix)
	ld	h, -1911(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85054
	ld	hl, #0
	jp	__cmp_e_14030
__cmp_t_85054:
	ld	hl, #1
__cmp_e_14030:
	dec	sp
	dec	sp
	ld	-1914(ix), l
	ld	-1913(ix), h
	ld	l, -1914(ix)
	ld	h, -1913(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_54098
	ld	hl, #0
	jp	__cmp_e_35325
__cmp_t_54098:
	ld	hl, #1
__cmp_e_35325:
	dec	sp
	dec	sp
	ld	-1916(ix), l
	ld	-1915(ix), h
	ld	l, -1916(ix)
	ld	h, -1915(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L483
	jp	__xcc_L484
__xcc_L484:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1918(ix), l
	ld	-1917(ix), h
	ld	l, -1918(ix)
	ld	h, -1917(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1920(ix), l
	ld	-1919(ix), h
	ld	l, -1920(ix)
	ld	h, -1919(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41081
	ld	hl, #0
	jp	__cmp_e_87516
__cmp_t_41081:
	ld	hl, #1
__cmp_e_87516:
	dec	sp
	dec	sp
	ld	-1922(ix), l
	ld	-1921(ix), h
	ld	l, -1922(ix)
	ld	h, -1921(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_53516
	ld	hl, #0
	jp	__cmp_e_33002
__cmp_t_53516:
	ld	hl, #1
__cmp_e_33002:
	dec	sp
	dec	sp
	ld	-1924(ix), l
	ld	-1923(ix), h
	jp	__xcc_L485
__xcc_L483:
	ld	hl, #1
	ld	-1924(ix), l
	ld	-1923(ix), h
__xcc_L485:
	ld	l, -1924(ix)
	ld	h, -1923(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L480
	jp	__xcc_L481
__xcc_L480:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1926(ix), l
	ld	-1925(ix), h
	ld	l, -1926(ix)
	ld	h, -1925(ix)
	dec	sp
	dec	sp
	ld	-1928(ix), l
	ld	-1927(ix), h
	jp	__xcc_L482
__xcc_L481:
	ld	hl, #1
	ld	-1928(ix), l
	ld	-1927(ix), h
__xcc_L482:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-1930(ix), l
	ld	-1929(ix), h
	.globl __mul16
	ld	l, -1930(ix)
	ld	h, -1929(ix)
	push	hl
	ld	l, -1928(ix)
	ld	h, -1927(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1932(ix), l
	ld	-1931(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1934(ix), l
	ld	-1933(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1942(ix), l
	ld	-1941(ix), h
	ld	l, -1934(ix)
	ld	h, -1933(ix)
	push	hl
	ld	l, -1942(ix)
	ld	h, -1941(ix)
	ld	b, l
	pop	hl
__shift_2231:
	ld	a, b
	or	a, a
	jp	z, __sdone_6139
	add	hl, hl
	djnz	__shift_2231
__sdone_6139:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1950(ix), l
	ld	-1949(ix), h
	ld	l, -1950(ix)
	ld	h, -1949(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_61796
	ld	hl, #0
	jp	__cmp_e_85404
__cmp_t_61796:
	ld	hl, #1
__cmp_e_85404:
	dec	sp
	dec	sp
	ld	-1952(ix), l
	ld	-1951(ix), h
	ld	l, -1952(ix)
	ld	h, -1951(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_82338
	ld	hl, #0
	jp	__cmp_e_74580
__cmp_t_82338:
	ld	hl, #1
__cmp_e_74580:
	dec	sp
	dec	sp
	ld	-1954(ix), l
	ld	-1953(ix), h
	ld	l, -1954(ix)
	ld	h, -1953(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L489
	jp	__xcc_L490
__xcc_L490:
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-1956(ix), l
	ld	-1955(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1964(ix), l
	ld	-1963(ix), h
	ld	l, -1956(ix)
	ld	h, -1955(ix)
	push	hl
	ld	l, -1964(ix)
	ld	h, -1963(ix)
	ld	b, l
	pop	hl
__shift_9218:
	ld	a, b
	or	a, a
	jp	z, __sdone_9021
	add	hl, hl
	djnz	__shift_9218
__sdone_9021:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1972(ix), l
	ld	-1971(ix), h
	ld	l, -1972(ix)
	ld	h, -1971(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1974(ix), l
	ld	-1973(ix), h
	ld	l, -1974(ix)
	ld	h, -1973(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13970
	ld	hl, #0
	jp	__cmp_e_39862
__cmp_t_13970:
	ld	hl, #1
__cmp_e_39862:
	dec	sp
	dec	sp
	ld	-1976(ix), l
	ld	-1975(ix), h
	ld	l, -1976(ix)
	ld	h, -1975(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_84812
	ld	hl, #0
	jp	__cmp_e_35379
__cmp_t_84812:
	ld	hl, #1
__cmp_e_35379:
	dec	sp
	dec	sp
	ld	-1978(ix), l
	ld	-1977(ix), h
	jp	__xcc_L491
__xcc_L489:
	ld	hl, #1
	ld	-1978(ix), l
	ld	-1977(ix), h
__xcc_L491:
	ld	l, -1978(ix)
	ld	h, -1977(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L486
	jp	__xcc_L487
__xcc_L486:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-1980(ix), l
	ld	-1979(ix), h
	ld	l, -1980(ix)
	ld	h, -1979(ix)
	dec	sp
	dec	sp
	ld	-1982(ix), l
	ld	-1981(ix), h
	jp	__xcc_L488
__xcc_L487:
	ld	hl, #1
	ld	-1982(ix), l
	ld	-1981(ix), h
__xcc_L488:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-1984(ix), l
	ld	-1983(ix), h
	.globl __mul16
	ld	l, -1984(ix)
	ld	h, -1983(ix)
	push	hl
	ld	l, -1982(ix)
	ld	h, -1981(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-1986(ix), l
	ld	-1985(ix), h
	ld	l, -1986(ix)
	ld	h, -1985(ix)
	push	hl
	ld	l, -1932(ix)
	ld	h, -1931(ix)
	push	hl
	ld	l, -1910(ix)
	ld	h, -1909(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L464:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L463
	jp	__xcc_L465
__xcc_L465:
__xcc_L374:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L373
	jp	__xcc_L375
__xcc_L375:
__xcc_L252:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L251
	jp	__xcc_L253
__xcc_L253:
__xcc_L492:
__xcc_L495:
__xcc_L498:
	ld	hl, #__str_501
	dec	sp
	dec	sp
	ld	-1988(ix), l
	ld	-1987(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-1992(ix), l
	ld	-1991(ix), h
	ld	l, -1992(ix)
	ld	h, -1991(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_94977
	ld	hl, #0
	jp	__cmp_e_92685
__cmp_t_94977:
	ld	hl, #1
__cmp_e_92685:
	dec	sp
	dec	sp
	ld	-1994(ix), l
	ld	-1993(ix), h
	ld	l, -1994(ix)
	ld	h, -1993(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_31536
	ld	hl, #0
	jp	__cmp_e_99904
__cmp_t_31536:
	ld	hl, #1
__cmp_e_99904:
	dec	sp
	dec	sp
	ld	-1996(ix), l
	ld	-1995(ix), h
	ld	l, -1996(ix)
	ld	h, -1995(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L505
	jp	__xcc_L506
__xcc_L506:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2000(ix), l
	ld	-1999(ix), h
	ld	l, -2000(ix)
	ld	h, -1999(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2002(ix), l
	ld	-2001(ix), h
	ld	l, -2002(ix)
	ld	h, -2001(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_24176
	ld	hl, #0
	jp	__cmp_e_33483
__cmp_t_24176:
	ld	hl, #1
__cmp_e_33483:
	dec	sp
	dec	sp
	ld	-2004(ix), l
	ld	-2003(ix), h
	ld	l, -2004(ix)
	ld	h, -2003(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79207
	ld	hl, #0
	jp	__cmp_e_99759
__cmp_t_79207:
	ld	hl, #1
__cmp_e_99759:
	dec	sp
	dec	sp
	ld	-2006(ix), l
	ld	-2005(ix), h
	jp	__xcc_L507
__xcc_L505:
	ld	hl, #1
	ld	-2006(ix), l
	ld	-2005(ix), h
__xcc_L507:
	ld	l, -2006(ix)
	ld	h, -2005(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L502
	jp	__xcc_L503
__xcc_L502:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2008(ix), l
	ld	-2007(ix), h
	ld	l, -2008(ix)
	ld	h, -2007(ix)
	dec	sp
	dec	sp
	ld	-2010(ix), l
	ld	-2009(ix), h
	jp	__xcc_L504
__xcc_L503:
	ld	hl, #1
	ld	-2010(ix), l
	ld	-2009(ix), h
__xcc_L504:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2012(ix), l
	ld	-2011(ix), h
	.globl __mul16
	ld	l, -2012(ix)
	ld	h, -2011(ix)
	push	hl
	ld	l, -2010(ix)
	ld	h, -2009(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2014(ix), l
	ld	-2013(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2018(ix), l
	ld	-2017(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2020(ix), l
	ld	-2019(ix), h
	ld	l, -2018(ix)
	ld	h, -2017(ix)
	push	hl
	ld	l, -2020(ix)
	ld	h, -2019(ix)
	ld	b, l
	pop	hl
__shift_4857:
	ld	a, b
	or	a, a
	jp	z, __sdone_9744
	add	hl, hl
	djnz	__shift_4857
__sdone_9744:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2024(ix), l
	ld	-2023(ix), h
	ld	l, -2024(ix)
	ld	h, -2023(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_93499
	ld	hl, #0
	jp	__cmp_e_69911
__cmp_t_93499:
	ld	hl, #1
__cmp_e_69911:
	dec	sp
	dec	sp
	ld	-2026(ix), l
	ld	-2025(ix), h
	ld	l, -2026(ix)
	ld	h, -2025(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_20127
	ld	hl, #0
	jp	__cmp_e_63950
__cmp_t_20127:
	ld	hl, #1
__cmp_e_63950:
	dec	sp
	dec	sp
	ld	-2028(ix), l
	ld	-2027(ix), h
	ld	l, -2028(ix)
	ld	h, -2027(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L511
	jp	__xcc_L512
__xcc_L512:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2032(ix), l
	ld	-2031(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2034(ix), l
	ld	-2033(ix), h
	ld	l, -2032(ix)
	ld	h, -2031(ix)
	push	hl
	ld	l, -2034(ix)
	ld	h, -2033(ix)
	ld	b, l
	pop	hl
__shift_5236:
	ld	a, b
	or	a, a
	jp	z, __sdone_7560
	add	hl, hl
	djnz	__shift_5236
__sdone_7560:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2038(ix), l
	ld	-2037(ix), h
	ld	l, -2038(ix)
	ld	h, -2037(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2040(ix), l
	ld	-2039(ix), h
	ld	l, -2040(ix)
	ld	h, -2039(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_67818
	ld	hl, #0
	jp	__cmp_e_75105
__cmp_t_67818:
	ld	hl, #1
__cmp_e_75105:
	dec	sp
	dec	sp
	ld	-2042(ix), l
	ld	-2041(ix), h
	ld	l, -2042(ix)
	ld	h, -2041(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10563
	ld	hl, #0
	jp	__cmp_e_40049
__cmp_t_10563:
	ld	hl, #1
__cmp_e_40049:
	dec	sp
	dec	sp
	ld	-2044(ix), l
	ld	-2043(ix), h
	jp	__xcc_L513
__xcc_L511:
	ld	hl, #1
	ld	-2044(ix), l
	ld	-2043(ix), h
__xcc_L513:
	ld	l, -2044(ix)
	ld	h, -2043(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L508
	jp	__xcc_L509
__xcc_L508:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2046(ix), l
	ld	-2045(ix), h
	ld	l, -2046(ix)
	ld	h, -2045(ix)
	dec	sp
	dec	sp
	ld	-2048(ix), l
	ld	-2047(ix), h
	jp	__xcc_L510
__xcc_L509:
	ld	hl, #1
	ld	-2048(ix), l
	ld	-2047(ix), h
__xcc_L510:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2050(ix), l
	ld	-2049(ix), h
	.globl __mul16
	ld	l, -2050(ix)
	ld	h, -2049(ix)
	push	hl
	ld	l, -2048(ix)
	ld	h, -2047(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2052(ix), l
	ld	-2051(ix), h
	ld	l, -2052(ix)
	ld	h, -2051(ix)
	push	hl
	ld	l, -2014(ix)
	ld	h, -2013(ix)
	push	hl
	ld	l, -1988(ix)
	ld	h, -1987(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_514
	dec	sp
	dec	sp
	ld	-2054(ix), l
	ld	-2053(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2058(ix), l
	ld	-2057(ix), h
	ld	l, -2058(ix)
	ld	h, -2057(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_21244
	ld	hl, #0
	jp	__cmp_e_88711
__cmp_t_21244:
	ld	hl, #1
__cmp_e_88711:
	dec	sp
	dec	sp
	ld	-2060(ix), l
	ld	-2059(ix), h
	ld	l, -2060(ix)
	ld	h, -2059(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41805
	ld	hl, #0
	jp	__cmp_e_19934
__cmp_t_41805:
	ld	hl, #1
__cmp_e_19934:
	dec	sp
	dec	sp
	ld	-2062(ix), l
	ld	-2061(ix), h
	ld	l, -2062(ix)
	ld	h, -2061(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L518
	jp	__xcc_L519
__xcc_L519:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2066(ix), l
	ld	-2065(ix), h
	ld	l, -2066(ix)
	ld	h, -2065(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2068(ix), l
	ld	-2067(ix), h
	ld	l, -2068(ix)
	ld	h, -2067(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63291
	ld	hl, #0
	jp	__cmp_e_77375
__cmp_t_63291:
	ld	hl, #1
__cmp_e_77375:
	dec	sp
	dec	sp
	ld	-2070(ix), l
	ld	-2069(ix), h
	ld	l, -2070(ix)
	ld	h, -2069(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58955
	ld	hl, #0
	jp	__cmp_e_93614
__cmp_t_58955:
	ld	hl, #1
__cmp_e_93614:
	dec	sp
	dec	sp
	ld	-2072(ix), l
	ld	-2071(ix), h
	jp	__xcc_L520
__xcc_L518:
	ld	hl, #1
	ld	-2072(ix), l
	ld	-2071(ix), h
__xcc_L520:
	ld	l, -2072(ix)
	ld	h, -2071(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L515
	jp	__xcc_L516
__xcc_L515:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2074(ix), l
	ld	-2073(ix), h
	ld	l, -2074(ix)
	ld	h, -2073(ix)
	dec	sp
	dec	sp
	ld	-2076(ix), l
	ld	-2075(ix), h
	jp	__xcc_L517
__xcc_L516:
	ld	hl, #1
	ld	-2076(ix), l
	ld	-2075(ix), h
__xcc_L517:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2078(ix), l
	ld	-2077(ix), h
	.globl __mul16
	ld	l, -2078(ix)
	ld	h, -2077(ix)
	push	hl
	ld	l, -2076(ix)
	ld	h, -2075(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2080(ix), l
	ld	-2079(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2084(ix), l
	ld	-2083(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2086(ix), l
	ld	-2085(ix), h
	ld	l, -2084(ix)
	ld	h, -2083(ix)
	push	hl
	ld	l, -2086(ix)
	ld	h, -2085(ix)
	ld	b, l
	pop	hl
__shift_3589:
	ld	a, b
	or	a, a
	jp	z, __sdone_3768
	add	hl, hl
	djnz	__shift_3589
__sdone_3768:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2090(ix), l
	ld	-2089(ix), h
	ld	l, -2090(ix)
	ld	h, -2089(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28993
	ld	hl, #0
	jp	__cmp_e_44918
__cmp_t_28993:
	ld	hl, #1
__cmp_e_44918:
	dec	sp
	dec	sp
	ld	-2092(ix), l
	ld	-2091(ix), h
	ld	l, -2092(ix)
	ld	h, -2091(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52805
	ld	hl, #0
	jp	__cmp_e_76882
__cmp_t_52805:
	ld	hl, #1
__cmp_e_76882:
	dec	sp
	dec	sp
	ld	-2094(ix), l
	ld	-2093(ix), h
	ld	l, -2094(ix)
	ld	h, -2093(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L524
	jp	__xcc_L525
__xcc_L525:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2098(ix), l
	ld	-2097(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2100(ix), l
	ld	-2099(ix), h
	ld	l, -2098(ix)
	ld	h, -2097(ix)
	push	hl
	ld	l, -2100(ix)
	ld	h, -2099(ix)
	ld	b, l
	pop	hl
__shift_4822:
	ld	a, b
	or	a, a
	jp	z, __sdone_6982
	add	hl, hl
	djnz	__shift_4822
__sdone_6982:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2104(ix), l
	ld	-2103(ix), h
	ld	l, -2104(ix)
	ld	h, -2103(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2106(ix), l
	ld	-2105(ix), h
	ld	l, -2106(ix)
	ld	h, -2105(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_26717
	ld	hl, #0
	jp	__cmp_e_24030
__cmp_t_26717:
	ld	hl, #1
__cmp_e_24030:
	dec	sp
	dec	sp
	ld	-2108(ix), l
	ld	-2107(ix), h
	ld	l, -2108(ix)
	ld	h, -2107(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93093
	ld	hl, #0
	jp	__cmp_e_11574
__cmp_t_93093:
	ld	hl, #1
__cmp_e_11574:
	dec	sp
	dec	sp
	ld	-2110(ix), l
	ld	-2109(ix), h
	jp	__xcc_L526
__xcc_L524:
	ld	hl, #1
	ld	-2110(ix), l
	ld	-2109(ix), h
__xcc_L526:
	ld	l, -2110(ix)
	ld	h, -2109(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L521
	jp	__xcc_L522
__xcc_L521:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2112(ix), l
	ld	-2111(ix), h
	ld	l, -2112(ix)
	ld	h, -2111(ix)
	dec	sp
	dec	sp
	ld	-2114(ix), l
	ld	-2113(ix), h
	jp	__xcc_L523
__xcc_L522:
	ld	hl, #1
	ld	-2114(ix), l
	ld	-2113(ix), h
__xcc_L523:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2116(ix), l
	ld	-2115(ix), h
	.globl __mul16
	ld	l, -2116(ix)
	ld	h, -2115(ix)
	push	hl
	ld	l, -2114(ix)
	ld	h, -2113(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2118(ix), l
	ld	-2117(ix), h
	ld	l, -2118(ix)
	ld	h, -2117(ix)
	push	hl
	ld	l, -2080(ix)
	ld	h, -2079(ix)
	push	hl
	ld	l, -2054(ix)
	ld	h, -2053(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L499:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L498
	jp	__xcc_L500
__xcc_L500:
__xcc_L527:
	ld	hl, #__str_530
	dec	sp
	dec	sp
	ld	-2120(ix), l
	ld	-2119(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2124(ix), l
	ld	-2123(ix), h
	ld	l, -2124(ix)
	ld	h, -2123(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30126
	ld	hl, #0
	jp	__cmp_e_86593
__cmp_t_30126:
	ld	hl, #1
__cmp_e_86593:
	dec	sp
	dec	sp
	ld	-2126(ix), l
	ld	-2125(ix), h
	ld	l, -2126(ix)
	ld	h, -2125(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81486
	ld	hl, #0
	jp	__cmp_e_50253
__cmp_t_81486:
	ld	hl, #1
__cmp_e_50253:
	dec	sp
	dec	sp
	ld	-2128(ix), l
	ld	-2127(ix), h
	ld	l, -2128(ix)
	ld	h, -2127(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L534
	jp	__xcc_L535
__xcc_L535:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2132(ix), l
	ld	-2131(ix), h
	ld	l, -2132(ix)
	ld	h, -2131(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2134(ix), l
	ld	-2133(ix), h
	ld	l, -2134(ix)
	ld	h, -2133(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_50543
	ld	hl, #0
	jp	__cmp_e_3074
__cmp_t_50543:
	ld	hl, #1
__cmp_e_3074:
	dec	sp
	dec	sp
	ld	-2136(ix), l
	ld	-2135(ix), h
	ld	l, -2136(ix)
	ld	h, -2135(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_27814
	ld	hl, #0
	jp	__cmp_e_34713
__cmp_t_27814:
	ld	hl, #1
__cmp_e_34713:
	dec	sp
	dec	sp
	ld	-2138(ix), l
	ld	-2137(ix), h
	jp	__xcc_L536
__xcc_L534:
	ld	hl, #1
	ld	-2138(ix), l
	ld	-2137(ix), h
__xcc_L536:
	ld	l, -2138(ix)
	ld	h, -2137(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L531
	jp	__xcc_L532
__xcc_L531:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2140(ix), l
	ld	-2139(ix), h
	ld	l, -2140(ix)
	ld	h, -2139(ix)
	dec	sp
	dec	sp
	ld	-2142(ix), l
	ld	-2141(ix), h
	jp	__xcc_L533
__xcc_L532:
	ld	hl, #1
	ld	-2142(ix), l
	ld	-2141(ix), h
__xcc_L533:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2144(ix), l
	ld	-2143(ix), h
	.globl __mul16
	ld	l, -2144(ix)
	ld	h, -2143(ix)
	push	hl
	ld	l, -2142(ix)
	ld	h, -2141(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2146(ix), l
	ld	-2145(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2150(ix), l
	ld	-2149(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2152(ix), l
	ld	-2151(ix), h
	ld	l, -2150(ix)
	ld	h, -2149(ix)
	push	hl
	ld	l, -2152(ix)
	ld	h, -2151(ix)
	ld	b, l
	pop	hl
__shift_8179:
	ld	a, b
	or	a, a
	jp	z, __sdone_8377
	add	hl, hl
	djnz	__shift_8179
__sdone_8377:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2156(ix), l
	ld	-2155(ix), h
	ld	l, -2156(ix)
	ld	h, -2155(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74762
	ld	hl, #0
	jp	__cmp_e_15775
__cmp_t_74762:
	ld	hl, #1
__cmp_e_15775:
	dec	sp
	dec	sp
	ld	-2158(ix), l
	ld	-2157(ix), h
	ld	l, -2158(ix)
	ld	h, -2157(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_27088
	ld	hl, #0
	jp	__cmp_e_32919
__cmp_t_27088:
	ld	hl, #1
__cmp_e_32919:
	dec	sp
	dec	sp
	ld	-2160(ix), l
	ld	-2159(ix), h
	ld	l, -2160(ix)
	ld	h, -2159(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L540
	jp	__xcc_L541
__xcc_L541:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2164(ix), l
	ld	-2163(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2166(ix), l
	ld	-2165(ix), h
	ld	l, -2164(ix)
	ld	h, -2163(ix)
	push	hl
	ld	l, -2166(ix)
	ld	h, -2165(ix)
	ld	b, l
	pop	hl
__shift_5710:
	ld	a, b
	or	a, a
	jp	z, __sdone_6732
	add	hl, hl
	djnz	__shift_5710
__sdone_6732:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2170(ix), l
	ld	-2169(ix), h
	ld	l, -2170(ix)
	ld	h, -2169(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2172(ix), l
	ld	-2171(ix), h
	ld	l, -2172(ix)
	ld	h, -2171(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_10294
	ld	hl, #0
	jp	__cmp_e_11017
__cmp_t_10294:
	ld	hl, #1
__cmp_e_11017:
	dec	sp
	dec	sp
	ld	-2174(ix), l
	ld	-2173(ix), h
	ld	l, -2174(ix)
	ld	h, -2173(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_346
	ld	hl, #0
	jp	__cmp_e_60235
__cmp_t_346:
	ld	hl, #1
__cmp_e_60235:
	dec	sp
	dec	sp
	ld	-2176(ix), l
	ld	-2175(ix), h
	jp	__xcc_L542
__xcc_L540:
	ld	hl, #1
	ld	-2176(ix), l
	ld	-2175(ix), h
__xcc_L542:
	ld	l, -2176(ix)
	ld	h, -2175(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L537
	jp	__xcc_L538
__xcc_L537:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2178(ix), l
	ld	-2177(ix), h
	ld	l, -2178(ix)
	ld	h, -2177(ix)
	dec	sp
	dec	sp
	ld	-2180(ix), l
	ld	-2179(ix), h
	jp	__xcc_L539
__xcc_L538:
	ld	hl, #1
	ld	-2180(ix), l
	ld	-2179(ix), h
__xcc_L539:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2182(ix), l
	ld	-2181(ix), h
	.globl __mul16
	ld	l, -2182(ix)
	ld	h, -2181(ix)
	push	hl
	ld	l, -2180(ix)
	ld	h, -2179(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2184(ix), l
	ld	-2183(ix), h
	ld	l, -2184(ix)
	ld	h, -2183(ix)
	push	hl
	ld	l, -2146(ix)
	ld	h, -2145(ix)
	push	hl
	ld	l, -2120(ix)
	ld	h, -2119(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_543
	dec	sp
	dec	sp
	ld	-2186(ix), l
	ld	-2185(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2190(ix), l
	ld	-2189(ix), h
	ld	l, -2190(ix)
	ld	h, -2189(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71137
	ld	hl, #0
	jp	__cmp_e_45691
__cmp_t_71137:
	ld	hl, #1
__cmp_e_45691:
	dec	sp
	dec	sp
	ld	-2192(ix), l
	ld	-2191(ix), h
	ld	l, -2192(ix)
	ld	h, -2191(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_5153
	ld	hl, #0
	jp	__cmp_e_23943
__cmp_t_5153:
	ld	hl, #1
__cmp_e_23943:
	dec	sp
	dec	sp
	ld	-2194(ix), l
	ld	-2193(ix), h
	ld	l, -2194(ix)
	ld	h, -2193(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L547
	jp	__xcc_L548
__xcc_L548:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2198(ix), l
	ld	-2197(ix), h
	ld	l, -2198(ix)
	ld	h, -2197(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2200(ix), l
	ld	-2199(ix), h
	ld	l, -2200(ix)
	ld	h, -2199(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22573
	ld	hl, #0
	jp	__cmp_e_66328
__cmp_t_22573:
	ld	hl, #1
__cmp_e_66328:
	dec	sp
	dec	sp
	ld	-2202(ix), l
	ld	-2201(ix), h
	ld	l, -2202(ix)
	ld	h, -2201(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_925
	ld	hl, #0
	jp	__cmp_e_49291
__cmp_t_925:
	ld	hl, #1
__cmp_e_49291:
	dec	sp
	dec	sp
	ld	-2204(ix), l
	ld	-2203(ix), h
	jp	__xcc_L549
__xcc_L547:
	ld	hl, #1
	ld	-2204(ix), l
	ld	-2203(ix), h
__xcc_L549:
	ld	l, -2204(ix)
	ld	h, -2203(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L544
	jp	__xcc_L545
__xcc_L544:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2206(ix), l
	ld	-2205(ix), h
	ld	l, -2206(ix)
	ld	h, -2205(ix)
	dec	sp
	dec	sp
	ld	-2208(ix), l
	ld	-2207(ix), h
	jp	__xcc_L546
__xcc_L545:
	ld	hl, #1
	ld	-2208(ix), l
	ld	-2207(ix), h
__xcc_L546:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2210(ix), l
	ld	-2209(ix), h
	.globl __mul16
	ld	l, -2210(ix)
	ld	h, -2209(ix)
	push	hl
	ld	l, -2208(ix)
	ld	h, -2207(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2212(ix), l
	ld	-2211(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2216(ix), l
	ld	-2215(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2218(ix), l
	ld	-2217(ix), h
	ld	l, -2216(ix)
	ld	h, -2215(ix)
	push	hl
	ld	l, -2218(ix)
	ld	h, -2217(ix)
	ld	b, l
	pop	hl
__shift_6710:
	ld	a, b
	or	a, a
	jp	z, __sdone_4018
	add	hl, hl
	djnz	__shift_6710
__sdone_4018:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2222(ix), l
	ld	-2221(ix), h
	ld	l, -2222(ix)
	ld	h, -2221(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_77217
	ld	hl, #0
	jp	__cmp_e_36836
__cmp_t_77217:
	ld	hl, #1
__cmp_e_36836:
	dec	sp
	dec	sp
	ld	-2224(ix), l
	ld	-2223(ix), h
	ld	l, -2224(ix)
	ld	h, -2223(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96963
	ld	hl, #0
	jp	__cmp_e_75055
__cmp_t_96963:
	ld	hl, #1
__cmp_e_75055:
	dec	sp
	dec	sp
	ld	-2226(ix), l
	ld	-2225(ix), h
	ld	l, -2226(ix)
	ld	h, -2225(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L553
	jp	__xcc_L554
__xcc_L554:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2230(ix), l
	ld	-2229(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2232(ix), l
	ld	-2231(ix), h
	ld	l, -2230(ix)
	ld	h, -2229(ix)
	push	hl
	ld	l, -2232(ix)
	ld	h, -2231(ix)
	ld	b, l
	pop	hl
__shift_7090:
	ld	a, b
	or	a, a
	jp	z, __sdone_3858
	add	hl, hl
	djnz	__shift_7090
__sdone_3858:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2236(ix), l
	ld	-2235(ix), h
	ld	l, -2236(ix)
	ld	h, -2235(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2238(ix), l
	ld	-2237(ix), h
	ld	l, -2238(ix)
	ld	h, -2237(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78130
	ld	hl, #0
	jp	__cmp_e_14904
__cmp_t_78130:
	ld	hl, #1
__cmp_e_14904:
	dec	sp
	dec	sp
	ld	-2240(ix), l
	ld	-2239(ix), h
	ld	l, -2240(ix)
	ld	h, -2239(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98571
	ld	hl, #0
	jp	__cmp_e_72661
__cmp_t_98571:
	ld	hl, #1
__cmp_e_72661:
	dec	sp
	dec	sp
	ld	-2242(ix), l
	ld	-2241(ix), h
	jp	__xcc_L555
__xcc_L553:
	ld	hl, #1
	ld	-2242(ix), l
	ld	-2241(ix), h
__xcc_L555:
	ld	l, -2242(ix)
	ld	h, -2241(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L550
	jp	__xcc_L551
__xcc_L550:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2244(ix), l
	ld	-2243(ix), h
	ld	l, -2244(ix)
	ld	h, -2243(ix)
	dec	sp
	dec	sp
	ld	-2246(ix), l
	ld	-2245(ix), h
	jp	__xcc_L552
__xcc_L551:
	ld	hl, #1
	ld	-2246(ix), l
	ld	-2245(ix), h
__xcc_L552:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2248(ix), l
	ld	-2247(ix), h
	.globl __mul16
	ld	l, -2248(ix)
	ld	h, -2247(ix)
	push	hl
	ld	l, -2246(ix)
	ld	h, -2245(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2250(ix), l
	ld	-2249(ix), h
	ld	l, -2250(ix)
	ld	h, -2249(ix)
	push	hl
	ld	l, -2212(ix)
	ld	h, -2211(ix)
	push	hl
	ld	l, -2186(ix)
	ld	h, -2185(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L528:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L527
	jp	__xcc_L529
__xcc_L529:
__xcc_L556:
	ld	hl, #__str_559
	dec	sp
	dec	sp
	ld	-2252(ix), l
	ld	-2251(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2256(ix), l
	ld	-2255(ix), h
	ld	l, -2256(ix)
	ld	h, -2255(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_69633
	ld	hl, #0
	jp	__cmp_e_89685
__cmp_t_69633:
	ld	hl, #1
__cmp_e_89685:
	dec	sp
	dec	sp
	ld	-2258(ix), l
	ld	-2257(ix), h
	ld	l, -2258(ix)
	ld	h, -2257(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4789
	ld	hl, #0
	jp	__cmp_e_13073
__cmp_t_4789:
	ld	hl, #1
__cmp_e_13073:
	dec	sp
	dec	sp
	ld	-2260(ix), l
	ld	-2259(ix), h
	ld	l, -2260(ix)
	ld	h, -2259(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L563
	jp	__xcc_L564
__xcc_L564:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2264(ix), l
	ld	-2263(ix), h
	ld	l, -2264(ix)
	ld	h, -2263(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2266(ix), l
	ld	-2265(ix), h
	ld	l, -2266(ix)
	ld	h, -2265(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22604
	ld	hl, #0
	jp	__cmp_e_56851
__cmp_t_22604:
	ld	hl, #1
__cmp_e_56851:
	dec	sp
	dec	sp
	ld	-2268(ix), l
	ld	-2267(ix), h
	ld	l, -2268(ix)
	ld	h, -2267(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19805
	ld	hl, #0
	jp	__cmp_e_49250
__cmp_t_19805:
	ld	hl, #1
__cmp_e_49250:
	dec	sp
	dec	sp
	ld	-2270(ix), l
	ld	-2269(ix), h
	jp	__xcc_L565
__xcc_L563:
	ld	hl, #1
	ld	-2270(ix), l
	ld	-2269(ix), h
__xcc_L565:
	ld	l, -2270(ix)
	ld	h, -2269(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L560
	jp	__xcc_L561
__xcc_L560:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2272(ix), l
	ld	-2271(ix), h
	ld	l, -2272(ix)
	ld	h, -2271(ix)
	dec	sp
	dec	sp
	ld	-2274(ix), l
	ld	-2273(ix), h
	jp	__xcc_L562
__xcc_L561:
	ld	hl, #1
	ld	-2274(ix), l
	ld	-2273(ix), h
__xcc_L562:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2276(ix), l
	ld	-2275(ix), h
	.globl __mul16
	ld	l, -2276(ix)
	ld	h, -2275(ix)
	push	hl
	ld	l, -2274(ix)
	ld	h, -2273(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2278(ix), l
	ld	-2277(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2282(ix), l
	ld	-2281(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2286(ix), l
	ld	-2285(ix), h
	ld	l, -2282(ix)
	ld	h, -2281(ix)
	push	hl
	ld	l, -2286(ix)
	ld	h, -2285(ix)
	ld	b, l
	pop	hl
__shift_7868:
	ld	a, b
	or	a, a
	jp	z, __sdone_6503
	add	hl, hl
	djnz	__shift_7868
__sdone_6503:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2290(ix), l
	ld	-2289(ix), h
	ld	l, -2290(ix)
	ld	h, -2289(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_9485
	ld	hl, #0
	jp	__cmp_e_39006
__cmp_t_9485:
	ld	hl, #1
__cmp_e_39006:
	dec	sp
	dec	sp
	ld	-2292(ix), l
	ld	-2291(ix), h
	ld	l, -2292(ix)
	ld	h, -2291(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_82195
	ld	hl, #0
	jp	__cmp_e_14639
__cmp_t_82195:
	ld	hl, #1
__cmp_e_14639:
	dec	sp
	dec	sp
	ld	-2294(ix), l
	ld	-2293(ix), h
	ld	l, -2294(ix)
	ld	h, -2293(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L569
	jp	__xcc_L570
__xcc_L570:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2298(ix), l
	ld	-2297(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2302(ix), l
	ld	-2301(ix), h
	ld	l, -2298(ix)
	ld	h, -2297(ix)
	push	hl
	ld	l, -2302(ix)
	ld	h, -2301(ix)
	ld	b, l
	pop	hl
__shift_2949:
	ld	a, b
	or	a, a
	jp	z, __sdone_1120
	add	hl, hl
	djnz	__shift_2949
__sdone_1120:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2306(ix), l
	ld	-2305(ix), h
	ld	l, -2306(ix)
	ld	h, -2305(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2308(ix), l
	ld	-2307(ix), h
	ld	l, -2308(ix)
	ld	h, -2307(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80967
	ld	hl, #0
	jp	__cmp_e_80226
__cmp_t_80967:
	ld	hl, #1
__cmp_e_80226:
	dec	sp
	dec	sp
	ld	-2310(ix), l
	ld	-2309(ix), h
	ld	l, -2310(ix)
	ld	h, -2309(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_86763
	ld	hl, #0
	jp	__cmp_e_87677
__cmp_t_86763:
	ld	hl, #1
__cmp_e_87677:
	dec	sp
	dec	sp
	ld	-2312(ix), l
	ld	-2311(ix), h
	jp	__xcc_L571
__xcc_L569:
	ld	hl, #1
	ld	-2312(ix), l
	ld	-2311(ix), h
__xcc_L571:
	ld	l, -2312(ix)
	ld	h, -2311(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L566
	jp	__xcc_L567
__xcc_L566:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2314(ix), l
	ld	-2313(ix), h
	ld	l, -2314(ix)
	ld	h, -2313(ix)
	dec	sp
	dec	sp
	ld	-2316(ix), l
	ld	-2315(ix), h
	jp	__xcc_L568
__xcc_L567:
	ld	hl, #1
	ld	-2316(ix), l
	ld	-2315(ix), h
__xcc_L568:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2318(ix), l
	ld	-2317(ix), h
	.globl __mul16
	ld	l, -2318(ix)
	ld	h, -2317(ix)
	push	hl
	ld	l, -2316(ix)
	ld	h, -2315(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2320(ix), l
	ld	-2319(ix), h
	ld	l, -2320(ix)
	ld	h, -2319(ix)
	push	hl
	ld	l, -2278(ix)
	ld	h, -2277(ix)
	push	hl
	ld	l, -2252(ix)
	ld	h, -2251(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_572
	dec	sp
	dec	sp
	ld	-2322(ix), l
	ld	-2321(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2326(ix), l
	ld	-2325(ix), h
	ld	l, -2326(ix)
	ld	h, -2325(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90596
	ld	hl, #0
	jp	__cmp_e_63981
__cmp_t_90596:
	ld	hl, #1
__cmp_e_63981:
	dec	sp
	dec	sp
	ld	-2328(ix), l
	ld	-2327(ix), h
	ld	l, -2328(ix)
	ld	h, -2327(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_40865
	ld	hl, #0
	jp	__cmp_e_87560
__cmp_t_40865:
	ld	hl, #1
__cmp_e_87560:
	dec	sp
	dec	sp
	ld	-2330(ix), l
	ld	-2329(ix), h
	ld	l, -2330(ix)
	ld	h, -2329(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L576
	jp	__xcc_L577
__xcc_L577:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2334(ix), l
	ld	-2333(ix), h
	ld	l, -2334(ix)
	ld	h, -2333(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2336(ix), l
	ld	-2335(ix), h
	ld	l, -2336(ix)
	ld	h, -2335(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39036
	ld	hl, #0
	jp	__cmp_e_27955
__cmp_t_39036:
	ld	hl, #1
__cmp_e_27955:
	dec	sp
	dec	sp
	ld	-2338(ix), l
	ld	-2337(ix), h
	ld	l, -2338(ix)
	ld	h, -2337(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_67770
	ld	hl, #0
	jp	__cmp_e_33518
__cmp_t_67770:
	ld	hl, #1
__cmp_e_33518:
	dec	sp
	dec	sp
	ld	-2340(ix), l
	ld	-2339(ix), h
	jp	__xcc_L578
__xcc_L576:
	ld	hl, #1
	ld	-2340(ix), l
	ld	-2339(ix), h
__xcc_L578:
	ld	l, -2340(ix)
	ld	h, -2339(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L573
	jp	__xcc_L574
__xcc_L573:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2342(ix), l
	ld	-2341(ix), h
	ld	l, -2342(ix)
	ld	h, -2341(ix)
	dec	sp
	dec	sp
	ld	-2344(ix), l
	ld	-2343(ix), h
	jp	__xcc_L575
__xcc_L574:
	ld	hl, #1
	ld	-2344(ix), l
	ld	-2343(ix), h
__xcc_L575:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2346(ix), l
	ld	-2345(ix), h
	.globl __mul16
	ld	l, -2346(ix)
	ld	h, -2345(ix)
	push	hl
	ld	l, -2344(ix)
	ld	h, -2343(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2348(ix), l
	ld	-2347(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2352(ix), l
	ld	-2351(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2356(ix), l
	ld	-2355(ix), h
	ld	l, -2352(ix)
	ld	h, -2351(ix)
	push	hl
	ld	l, -2356(ix)
	ld	h, -2355(ix)
	ld	b, l
	pop	hl
__shift_9211:
	ld	a, b
	or	a, a
	jp	z, __sdone_6342
	add	hl, hl
	djnz	__shift_9211
__sdone_6342:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2360(ix), l
	ld	-2359(ix), h
	ld	l, -2360(ix)
	ld	h, -2359(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22532
	ld	hl, #0
	jp	__cmp_e_45196
__cmp_t_22532:
	ld	hl, #1
__cmp_e_45196:
	dec	sp
	dec	sp
	ld	-2362(ix), l
	ld	-2361(ix), h
	ld	l, -2362(ix)
	ld	h, -2361(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_72379
	ld	hl, #0
	jp	__cmp_e_27321
__cmp_t_72379:
	ld	hl, #1
__cmp_e_27321:
	dec	sp
	dec	sp
	ld	-2364(ix), l
	ld	-2363(ix), h
	ld	l, -2364(ix)
	ld	h, -2363(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L582
	jp	__xcc_L583
__xcc_L583:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2368(ix), l
	ld	-2367(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2372(ix), l
	ld	-2371(ix), h
	ld	l, -2368(ix)
	ld	h, -2367(ix)
	push	hl
	ld	l, -2372(ix)
	ld	h, -2371(ix)
	ld	b, l
	pop	hl
__shift_8270:
	ld	a, b
	or	a, a
	jp	z, __sdone_4984
	add	hl, hl
	djnz	__shift_8270
__sdone_4984:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2376(ix), l
	ld	-2375(ix), h
	ld	l, -2376(ix)
	ld	h, -2375(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2378(ix), l
	ld	-2377(ix), h
	ld	l, -2378(ix)
	ld	h, -2377(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84172
	ld	hl, #0
	jp	__cmp_e_94427
__cmp_t_84172:
	ld	hl, #1
__cmp_e_94427:
	dec	sp
	dec	sp
	ld	-2380(ix), l
	ld	-2379(ix), h
	ld	l, -2380(ix)
	ld	h, -2379(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44234
	ld	hl, #0
	jp	__cmp_e_52040
__cmp_t_44234:
	ld	hl, #1
__cmp_e_52040:
	dec	sp
	dec	sp
	ld	-2382(ix), l
	ld	-2381(ix), h
	jp	__xcc_L584
__xcc_L582:
	ld	hl, #1
	ld	-2382(ix), l
	ld	-2381(ix), h
__xcc_L584:
	ld	l, -2382(ix)
	ld	h, -2381(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L579
	jp	__xcc_L580
__xcc_L579:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2384(ix), l
	ld	-2383(ix), h
	ld	l, -2384(ix)
	ld	h, -2383(ix)
	dec	sp
	dec	sp
	ld	-2386(ix), l
	ld	-2385(ix), h
	jp	__xcc_L581
__xcc_L580:
	ld	hl, #1
	ld	-2386(ix), l
	ld	-2385(ix), h
__xcc_L581:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2388(ix), l
	ld	-2387(ix), h
	.globl __mul16
	ld	l, -2388(ix)
	ld	h, -2387(ix)
	push	hl
	ld	l, -2386(ix)
	ld	h, -2385(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2390(ix), l
	ld	-2389(ix), h
	ld	l, -2390(ix)
	ld	h, -2389(ix)
	push	hl
	ld	l, -2348(ix)
	ld	h, -2347(ix)
	push	hl
	ld	l, -2322(ix)
	ld	h, -2321(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L557:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L556
	jp	__xcc_L558
__xcc_L558:
__xcc_L585:
	ld	hl, #__str_588
	dec	sp
	dec	sp
	ld	-2392(ix), l
	ld	-2391(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2396(ix), l
	ld	-2395(ix), h
	ld	l, -2396(ix)
	ld	h, -2395(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47283
	ld	hl, #0
	jp	__cmp_e_70072
__cmp_t_47283:
	ld	hl, #1
__cmp_e_70072:
	dec	sp
	dec	sp
	ld	-2398(ix), l
	ld	-2397(ix), h
	ld	l, -2398(ix)
	ld	h, -2397(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7398
	ld	hl, #0
	jp	__cmp_e_45830
__cmp_t_7398:
	ld	hl, #1
__cmp_e_45830:
	dec	sp
	dec	sp
	ld	-2400(ix), l
	ld	-2399(ix), h
	ld	l, -2400(ix)
	ld	h, -2399(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L592
	jp	__xcc_L593
__xcc_L593:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2404(ix), l
	ld	-2403(ix), h
	ld	l, -2404(ix)
	ld	h, -2403(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2406(ix), l
	ld	-2405(ix), h
	ld	l, -2406(ix)
	ld	h, -2405(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1063
	ld	hl, #0
	jp	__cmp_e_70347
__cmp_t_1063:
	ld	hl, #1
__cmp_e_70347:
	dec	sp
	dec	sp
	ld	-2408(ix), l
	ld	-2407(ix), h
	ld	l, -2408(ix)
	ld	h, -2407(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66950
	ld	hl, #0
	jp	__cmp_e_82030
__cmp_t_66950:
	ld	hl, #1
__cmp_e_82030:
	dec	sp
	dec	sp
	ld	-2410(ix), l
	ld	-2409(ix), h
	jp	__xcc_L594
__xcc_L592:
	ld	hl, #1
	ld	-2410(ix), l
	ld	-2409(ix), h
__xcc_L594:
	ld	l, -2410(ix)
	ld	h, -2409(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L589
	jp	__xcc_L590
__xcc_L589:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2412(ix), l
	ld	-2411(ix), h
	ld	l, -2412(ix)
	ld	h, -2411(ix)
	dec	sp
	dec	sp
	ld	-2414(ix), l
	ld	-2413(ix), h
	jp	__xcc_L591
__xcc_L590:
	ld	hl, #1
	ld	-2414(ix), l
	ld	-2413(ix), h
__xcc_L591:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2416(ix), l
	ld	-2415(ix), h
	.globl __mul16
	ld	l, -2416(ix)
	ld	h, -2415(ix)
	push	hl
	ld	l, -2414(ix)
	ld	h, -2413(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2418(ix), l
	ld	-2417(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2422(ix), l
	ld	-2421(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2430(ix), l
	ld	-2429(ix), h
	ld	l, -2422(ix)
	ld	h, -2421(ix)
	push	hl
	ld	l, -2430(ix)
	ld	h, -2429(ix)
	ld	b, l
	pop	hl
__shift_573:
	ld	a, b
	or	a, a
	jp	z, __sdone_3714
	add	hl, hl
	djnz	__shift_573
__sdone_3714:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2438(ix), l
	ld	-2437(ix), h
	ld	l, -2438(ix)
	ld	h, -2437(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_86059
	ld	hl, #0
	jp	__cmp_e_57522
__cmp_t_86059:
	ld	hl, #1
__cmp_e_57522:
	dec	sp
	dec	sp
	ld	-2440(ix), l
	ld	-2439(ix), h
	ld	l, -2440(ix)
	ld	h, -2439(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34047
	ld	hl, #0
	jp	__cmp_e_26924
__cmp_t_34047:
	ld	hl, #1
__cmp_e_26924:
	dec	sp
	dec	sp
	ld	-2442(ix), l
	ld	-2441(ix), h
	ld	l, -2442(ix)
	ld	h, -2441(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L598
	jp	__xcc_L599
__xcc_L599:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2446(ix), l
	ld	-2445(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2454(ix), l
	ld	-2453(ix), h
	ld	l, -2446(ix)
	ld	h, -2445(ix)
	push	hl
	ld	l, -2454(ix)
	ld	h, -2453(ix)
	ld	b, l
	pop	hl
__shift_5082:
	ld	a, b
	or	a, a
	jp	z, __sdone_9435
	add	hl, hl
	djnz	__shift_5082
__sdone_9435:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2462(ix), l
	ld	-2461(ix), h
	ld	l, -2462(ix)
	ld	h, -2461(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2464(ix), l
	ld	-2463(ix), h
	ld	l, -2464(ix)
	ld	h, -2463(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71232
	ld	hl, #0
	jp	__cmp_e_29204
__cmp_t_71232:
	ld	hl, #1
__cmp_e_29204:
	dec	sp
	dec	sp
	ld	-2466(ix), l
	ld	-2465(ix), h
	ld	l, -2466(ix)
	ld	h, -2465(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22954
	ld	hl, #0
	jp	__cmp_e_30443
__cmp_t_22954:
	ld	hl, #1
__cmp_e_30443:
	dec	sp
	dec	sp
	ld	-2468(ix), l
	ld	-2467(ix), h
	jp	__xcc_L600
__xcc_L598:
	ld	hl, #1
	ld	-2468(ix), l
	ld	-2467(ix), h
__xcc_L600:
	ld	l, -2468(ix)
	ld	h, -2467(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L595
	jp	__xcc_L596
__xcc_L595:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2470(ix), l
	ld	-2469(ix), h
	ld	l, -2470(ix)
	ld	h, -2469(ix)
	dec	sp
	dec	sp
	ld	-2472(ix), l
	ld	-2471(ix), h
	jp	__xcc_L597
__xcc_L596:
	ld	hl, #1
	ld	-2472(ix), l
	ld	-2471(ix), h
__xcc_L597:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-2474(ix), l
	ld	-2473(ix), h
	.globl __mul16
	ld	l, -2474(ix)
	ld	h, -2473(ix)
	push	hl
	ld	l, -2472(ix)
	ld	h, -2471(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2476(ix), l
	ld	-2475(ix), h
	ld	l, -2476(ix)
	ld	h, -2475(ix)
	push	hl
	ld	l, -2418(ix)
	ld	h, -2417(ix)
	push	hl
	ld	l, -2392(ix)
	ld	h, -2391(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_601
	dec	sp
	dec	sp
	ld	-2478(ix), l
	ld	-2477(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2482(ix), l
	ld	-2481(ix), h
	ld	l, -2482(ix)
	ld	h, -2481(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_11898
	ld	hl, #0
	jp	__cmp_e_45486
__cmp_t_11898:
	ld	hl, #1
__cmp_e_45486:
	dec	sp
	dec	sp
	ld	-2484(ix), l
	ld	-2483(ix), h
	ld	l, -2484(ix)
	ld	h, -2483(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75640
	ld	hl, #0
	jp	__cmp_e_84278
__cmp_t_75640:
	ld	hl, #1
__cmp_e_84278:
	dec	sp
	dec	sp
	ld	-2486(ix), l
	ld	-2485(ix), h
	ld	l, -2486(ix)
	ld	h, -2485(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L605
	jp	__xcc_L606
__xcc_L606:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2490(ix), l
	ld	-2489(ix), h
	ld	l, -2490(ix)
	ld	h, -2489(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2492(ix), l
	ld	-2491(ix), h
	ld	l, -2492(ix)
	ld	h, -2491(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89159
	ld	hl, #0
	jp	__cmp_e_50262
__cmp_t_89159:
	ld	hl, #1
__cmp_e_50262:
	dec	sp
	dec	sp
	ld	-2494(ix), l
	ld	-2493(ix), h
	ld	l, -2494(ix)
	ld	h, -2493(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79262
	ld	hl, #0
	jp	__cmp_e_89683
__cmp_t_79262:
	ld	hl, #1
__cmp_e_89683:
	dec	sp
	dec	sp
	ld	-2496(ix), l
	ld	-2495(ix), h
	jp	__xcc_L607
__xcc_L605:
	ld	hl, #1
	ld	-2496(ix), l
	ld	-2495(ix), h
__xcc_L607:
	ld	l, -2496(ix)
	ld	h, -2495(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L602
	jp	__xcc_L603
__xcc_L602:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2498(ix), l
	ld	-2497(ix), h
	ld	l, -2498(ix)
	ld	h, -2497(ix)
	dec	sp
	dec	sp
	ld	-2500(ix), l
	ld	-2499(ix), h
	jp	__xcc_L604
__xcc_L603:
	ld	hl, #1
	ld	-2500(ix), l
	ld	-2499(ix), h
__xcc_L604:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2502(ix), l
	ld	-2501(ix), h
	.globl __mul16
	ld	l, -2502(ix)
	ld	h, -2501(ix)
	push	hl
	ld	l, -2500(ix)
	ld	h, -2499(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2504(ix), l
	ld	-2503(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2508(ix), l
	ld	-2507(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2516(ix), l
	ld	-2515(ix), h
	ld	l, -2508(ix)
	ld	h, -2507(ix)
	push	hl
	ld	l, -2516(ix)
	ld	h, -2515(ix)
	ld	b, l
	pop	hl
__shift_1041:
	ld	a, b
	or	a, a
	jp	z, __sdone_9848
	add	hl, hl
	djnz	__shift_1041
__sdone_9848:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2524(ix), l
	ld	-2523(ix), h
	ld	l, -2524(ix)
	ld	h, -2523(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41723
	ld	hl, #0
	jp	__cmp_e_8324
__cmp_t_41723:
	ld	hl, #1
__cmp_e_8324:
	dec	sp
	dec	sp
	ld	-2526(ix), l
	ld	-2525(ix), h
	ld	l, -2526(ix)
	ld	h, -2525(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26272
	ld	hl, #0
	jp	__cmp_e_49122
__cmp_t_26272:
	ld	hl, #1
__cmp_e_49122:
	dec	sp
	dec	sp
	ld	-2528(ix), l
	ld	-2527(ix), h
	ld	l, -2528(ix)
	ld	h, -2527(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L611
	jp	__xcc_L612
__xcc_L612:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2532(ix), l
	ld	-2531(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2540(ix), l
	ld	-2539(ix), h
	ld	l, -2532(ix)
	ld	h, -2531(ix)
	push	hl
	ld	l, -2540(ix)
	ld	h, -2539(ix)
	ld	b, l
	pop	hl
__shift_4154:
	ld	a, b
	or	a, a
	jp	z, __sdone_7335
	add	hl, hl
	djnz	__shift_4154
__sdone_7335:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2548(ix), l
	ld	-2547(ix), h
	ld	l, -2548(ix)
	ld	h, -2547(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2550(ix), l
	ld	-2549(ix), h
	ld	l, -2550(ix)
	ld	h, -2549(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35821
	ld	hl, #0
	jp	__cmp_e_37457
__cmp_t_35821:
	ld	hl, #1
__cmp_e_37457:
	dec	sp
	dec	sp
	ld	-2552(ix), l
	ld	-2551(ix), h
	ld	l, -2552(ix)
	ld	h, -2551(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_9365
	ld	hl, #0
	jp	__cmp_e_2747
__cmp_t_9365:
	ld	hl, #1
__cmp_e_2747:
	dec	sp
	dec	sp
	ld	-2554(ix), l
	ld	-2553(ix), h
	jp	__xcc_L613
__xcc_L611:
	ld	hl, #1
	ld	-2554(ix), l
	ld	-2553(ix), h
__xcc_L613:
	ld	l, -2554(ix)
	ld	h, -2553(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L608
	jp	__xcc_L609
__xcc_L608:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2556(ix), l
	ld	-2555(ix), h
	ld	l, -2556(ix)
	ld	h, -2555(ix)
	dec	sp
	dec	sp
	ld	-2558(ix), l
	ld	-2557(ix), h
	jp	__xcc_L610
__xcc_L609:
	ld	hl, #1
	ld	-2558(ix), l
	ld	-2557(ix), h
__xcc_L610:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-2560(ix), l
	ld	-2559(ix), h
	.globl __mul16
	ld	l, -2560(ix)
	ld	h, -2559(ix)
	push	hl
	ld	l, -2558(ix)
	ld	h, -2557(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2562(ix), l
	ld	-2561(ix), h
	ld	l, -2562(ix)
	ld	h, -2561(ix)
	push	hl
	ld	l, -2504(ix)
	ld	h, -2503(ix)
	push	hl
	ld	l, -2478(ix)
	ld	h, -2477(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L586:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L585
	jp	__xcc_L587
__xcc_L587:
__xcc_L496:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L495
	jp	__xcc_L497
__xcc_L497:
__xcc_L614:
__xcc_L617:
	ld	hl, #__str_620
	dec	sp
	dec	sp
	ld	-2564(ix), l
	ld	-2563(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2568(ix), l
	ld	-2567(ix), h
	ld	l, -2568(ix)
	ld	h, -2567(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91171
	ld	hl, #0
	jp	__cmp_e_11776
__cmp_t_91171:
	ld	hl, #1
__cmp_e_11776:
	dec	sp
	dec	sp
	ld	-2570(ix), l
	ld	-2569(ix), h
	ld	l, -2570(ix)
	ld	h, -2569(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60269
	ld	hl, #0
	jp	__cmp_e_25218
__cmp_t_60269:
	ld	hl, #1
__cmp_e_25218:
	dec	sp
	dec	sp
	ld	-2572(ix), l
	ld	-2571(ix), h
	ld	l, -2572(ix)
	ld	h, -2571(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L624
	jp	__xcc_L625
__xcc_L625:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2576(ix), l
	ld	-2575(ix), h
	ld	l, -2576(ix)
	ld	h, -2575(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2578(ix), l
	ld	-2577(ix), h
	ld	l, -2578(ix)
	ld	h, -2577(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_38701
	ld	hl, #0
	jp	__cmp_e_21703
__cmp_t_38701:
	ld	hl, #1
__cmp_e_21703:
	dec	sp
	dec	sp
	ld	-2580(ix), l
	ld	-2579(ix), h
	ld	l, -2580(ix)
	ld	h, -2579(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14653
	ld	hl, #0
	jp	__cmp_e_9933
__cmp_t_14653:
	ld	hl, #1
__cmp_e_9933:
	dec	sp
	dec	sp
	ld	-2582(ix), l
	ld	-2581(ix), h
	jp	__xcc_L626
__xcc_L624:
	ld	hl, #1
	ld	-2582(ix), l
	ld	-2581(ix), h
__xcc_L626:
	ld	l, -2582(ix)
	ld	h, -2581(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L621
	jp	__xcc_L622
__xcc_L621:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2584(ix), l
	ld	-2583(ix), h
	ld	l, -2584(ix)
	ld	h, -2583(ix)
	dec	sp
	dec	sp
	ld	-2586(ix), l
	ld	-2585(ix), h
	jp	__xcc_L623
__xcc_L622:
	ld	hl, #1
	ld	-2586(ix), l
	ld	-2585(ix), h
__xcc_L623:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2588(ix), l
	ld	-2587(ix), h
	.globl __mul16
	ld	l, -2588(ix)
	ld	h, -2587(ix)
	push	hl
	ld	l, -2586(ix)
	ld	h, -2585(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2590(ix), l
	ld	-2589(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2594(ix), l
	ld	-2593(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2596(ix), l
	ld	-2595(ix), h
	ld	l, -2594(ix)
	ld	h, -2593(ix)
	push	hl
	ld	l, -2596(ix)
	ld	h, -2595(ix)
	ld	b, l
	pop	hl
__shift_907:
	ld	a, b
	or	a, a
	jp	z, __sdone_3959
	add	hl, hl
	djnz	__shift_907
__sdone_3959:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2600(ix), l
	ld	-2599(ix), h
	ld	l, -2600(ix)
	ld	h, -2599(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_56728
	ld	hl, #0
	jp	__cmp_e_62806
__cmp_t_56728:
	ld	hl, #1
__cmp_e_62806:
	dec	sp
	dec	sp
	ld	-2602(ix), l
	ld	-2601(ix), h
	ld	l, -2602(ix)
	ld	h, -2601(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15797
	ld	hl, #0
	jp	__cmp_e_48720
__cmp_t_15797:
	ld	hl, #1
__cmp_e_48720:
	dec	sp
	dec	sp
	ld	-2604(ix), l
	ld	-2603(ix), h
	ld	l, -2604(ix)
	ld	h, -2603(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L630
	jp	__xcc_L631
__xcc_L631:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2608(ix), l
	ld	-2607(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2610(ix), l
	ld	-2609(ix), h
	ld	l, -2608(ix)
	ld	h, -2607(ix)
	push	hl
	ld	l, -2610(ix)
	ld	h, -2609(ix)
	ld	b, l
	pop	hl
__shift_7084:
	ld	a, b
	or	a, a
	jp	z, __sdone_1308
	add	hl, hl
	djnz	__shift_7084
__sdone_1308:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2614(ix), l
	ld	-2613(ix), h
	ld	l, -2614(ix)
	ld	h, -2613(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2616(ix), l
	ld	-2615(ix), h
	ld	l, -2616(ix)
	ld	h, -2615(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_15334
	ld	hl, #0
	jp	__cmp_e_42698
__cmp_t_15334:
	ld	hl, #1
__cmp_e_42698:
	dec	sp
	dec	sp
	ld	-2618(ix), l
	ld	-2617(ix), h
	ld	l, -2618(ix)
	ld	h, -2617(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10991
	ld	hl, #0
	jp	__cmp_e_76376
__cmp_t_10991:
	ld	hl, #1
__cmp_e_76376:
	dec	sp
	dec	sp
	ld	-2620(ix), l
	ld	-2619(ix), h
	jp	__xcc_L632
__xcc_L630:
	ld	hl, #1
	ld	-2620(ix), l
	ld	-2619(ix), h
__xcc_L632:
	ld	l, -2620(ix)
	ld	h, -2619(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L627
	jp	__xcc_L628
__xcc_L627:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2622(ix), l
	ld	-2621(ix), h
	ld	l, -2622(ix)
	ld	h, -2621(ix)
	dec	sp
	dec	sp
	ld	-2624(ix), l
	ld	-2623(ix), h
	jp	__xcc_L629
__xcc_L628:
	ld	hl, #1
	ld	-2624(ix), l
	ld	-2623(ix), h
__xcc_L629:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2626(ix), l
	ld	-2625(ix), h
	.globl __mul16
	ld	l, -2626(ix)
	ld	h, -2625(ix)
	push	hl
	ld	l, -2624(ix)
	ld	h, -2623(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2628(ix), l
	ld	-2627(ix), h
	ld	l, -2628(ix)
	ld	h, -2627(ix)
	push	hl
	ld	l, -2590(ix)
	ld	h, -2589(ix)
	push	hl
	ld	l, -2564(ix)
	ld	h, -2563(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_633
	dec	sp
	dec	sp
	ld	-2630(ix), l
	ld	-2629(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2634(ix), l
	ld	-2633(ix), h
	ld	l, -2634(ix)
	ld	h, -2633(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98898
	ld	hl, #0
	jp	__cmp_e_52715
__cmp_t_98898:
	ld	hl, #1
__cmp_e_52715:
	dec	sp
	dec	sp
	ld	-2636(ix), l
	ld	-2635(ix), h
	ld	l, -2636(ix)
	ld	h, -2635(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_1052
	ld	hl, #0
	jp	__cmp_e_25171
__cmp_t_1052:
	ld	hl, #1
__cmp_e_25171:
	dec	sp
	dec	sp
	ld	-2638(ix), l
	ld	-2637(ix), h
	ld	l, -2638(ix)
	ld	h, -2637(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L637
	jp	__xcc_L638
__xcc_L638:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2642(ix), l
	ld	-2641(ix), h
	ld	l, -2642(ix)
	ld	h, -2641(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2644(ix), l
	ld	-2643(ix), h
	ld	l, -2644(ix)
	ld	h, -2643(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18189
	ld	hl, #0
	jp	__cmp_e_71559
__cmp_t_18189:
	ld	hl, #1
__cmp_e_71559:
	dec	sp
	dec	sp
	ld	-2646(ix), l
	ld	-2645(ix), h
	ld	l, -2646(ix)
	ld	h, -2645(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52506
	ld	hl, #0
	jp	__cmp_e_54010
__cmp_t_52506:
	ld	hl, #1
__cmp_e_54010:
	dec	sp
	dec	sp
	ld	-2648(ix), l
	ld	-2647(ix), h
	jp	__xcc_L639
__xcc_L637:
	ld	hl, #1
	ld	-2648(ix), l
	ld	-2647(ix), h
__xcc_L639:
	ld	l, -2648(ix)
	ld	h, -2647(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L634
	jp	__xcc_L635
__xcc_L634:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2650(ix), l
	ld	-2649(ix), h
	ld	l, -2650(ix)
	ld	h, -2649(ix)
	dec	sp
	dec	sp
	ld	-2652(ix), l
	ld	-2651(ix), h
	jp	__xcc_L636
__xcc_L635:
	ld	hl, #1
	ld	-2652(ix), l
	ld	-2651(ix), h
__xcc_L636:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2654(ix), l
	ld	-2653(ix), h
	.globl __mul16
	ld	l, -2654(ix)
	ld	h, -2653(ix)
	push	hl
	ld	l, -2652(ix)
	ld	h, -2651(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2656(ix), l
	ld	-2655(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2660(ix), l
	ld	-2659(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2662(ix), l
	ld	-2661(ix), h
	ld	l, -2660(ix)
	ld	h, -2659(ix)
	push	hl
	ld	l, -2662(ix)
	ld	h, -2661(ix)
	ld	b, l
	pop	hl
__shift_9016:
	ld	a, b
	or	a, a
	jp	z, __sdone_8224
	add	hl, hl
	djnz	__shift_9016
__sdone_8224:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2666(ix), l
	ld	-2665(ix), h
	ld	l, -2666(ix)
	ld	h, -2665(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73109
	ld	hl, #0
	jp	__cmp_e_16539
__cmp_t_73109:
	ld	hl, #1
__cmp_e_16539:
	dec	sp
	dec	sp
	ld	-2668(ix), l
	ld	-2667(ix), h
	ld	l, -2668(ix)
	ld	h, -2667(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_90000
	ld	hl, #0
	jp	__cmp_e_33378
__cmp_t_90000:
	ld	hl, #1
__cmp_e_33378:
	dec	sp
	dec	sp
	ld	-2670(ix), l
	ld	-2669(ix), h
	ld	l, -2670(ix)
	ld	h, -2669(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L643
	jp	__xcc_L644
__xcc_L644:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2674(ix), l
	ld	-2673(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2676(ix), l
	ld	-2675(ix), h
	ld	l, -2674(ix)
	ld	h, -2673(ix)
	push	hl
	ld	l, -2676(ix)
	ld	h, -2675(ix)
	ld	b, l
	pop	hl
__shift_8109:
	ld	a, b
	or	a, a
	jp	z, __sdone_5053
	add	hl, hl
	djnz	__shift_8109
__sdone_5053:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2680(ix), l
	ld	-2679(ix), h
	ld	l, -2680(ix)
	ld	h, -2679(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2682(ix), l
	ld	-2681(ix), h
	ld	l, -2682(ix)
	ld	h, -2681(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_55081
	ld	hl, #0
	jp	__cmp_e_89114
__cmp_t_55081:
	ld	hl, #1
__cmp_e_89114:
	dec	sp
	dec	sp
	ld	-2684(ix), l
	ld	-2683(ix), h
	ld	l, -2684(ix)
	ld	h, -2683(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71338
	ld	hl, #0
	jp	__cmp_e_5989
__cmp_t_71338:
	ld	hl, #1
__cmp_e_5989:
	dec	sp
	dec	sp
	ld	-2686(ix), l
	ld	-2685(ix), h
	jp	__xcc_L645
__xcc_L643:
	ld	hl, #1
	ld	-2686(ix), l
	ld	-2685(ix), h
__xcc_L645:
	ld	l, -2686(ix)
	ld	h, -2685(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L640
	jp	__xcc_L641
__xcc_L640:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2688(ix), l
	ld	-2687(ix), h
	ld	l, -2688(ix)
	ld	h, -2687(ix)
	dec	sp
	dec	sp
	ld	-2690(ix), l
	ld	-2689(ix), h
	jp	__xcc_L642
__xcc_L641:
	ld	hl, #1
	ld	-2690(ix), l
	ld	-2689(ix), h
__xcc_L642:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2692(ix), l
	ld	-2691(ix), h
	.globl __mul16
	ld	l, -2692(ix)
	ld	h, -2691(ix)
	push	hl
	ld	l, -2690(ix)
	ld	h, -2689(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2694(ix), l
	ld	-2693(ix), h
	ld	l, -2694(ix)
	ld	h, -2693(ix)
	push	hl
	ld	l, -2656(ix)
	ld	h, -2655(ix)
	push	hl
	ld	l, -2630(ix)
	ld	h, -2629(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L618:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L617
	jp	__xcc_L619
__xcc_L619:
__xcc_L646:
	ld	hl, #__str_649
	dec	sp
	dec	sp
	ld	-2696(ix), l
	ld	-2695(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2700(ix), l
	ld	-2699(ix), h
	ld	l, -2700(ix)
	ld	h, -2699(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59426
	ld	hl, #0
	jp	__cmp_e_28067
__cmp_t_59426:
	ld	hl, #1
__cmp_e_28067:
	dec	sp
	dec	sp
	ld	-2702(ix), l
	ld	-2701(ix), h
	ld	l, -2702(ix)
	ld	h, -2701(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85147
	ld	hl, #0
	jp	__cmp_e_75223
__cmp_t_85147:
	ld	hl, #1
__cmp_e_75223:
	dec	sp
	dec	sp
	ld	-2704(ix), l
	ld	-2703(ix), h
	ld	l, -2704(ix)
	ld	h, -2703(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L653
	jp	__xcc_L654
__xcc_L654:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2708(ix), l
	ld	-2707(ix), h
	ld	l, -2708(ix)
	ld	h, -2707(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2710(ix), l
	ld	-2709(ix), h
	ld	l, -2710(ix)
	ld	h, -2709(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_76787
	ld	hl, #0
	jp	__cmp_e_32231
__cmp_t_76787:
	ld	hl, #1
__cmp_e_32231:
	dec	sp
	dec	sp
	ld	-2712(ix), l
	ld	-2711(ix), h
	ld	l, -2712(ix)
	ld	h, -2711(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96532
	ld	hl, #0
	jp	__cmp_e_92122
__cmp_t_96532:
	ld	hl, #1
__cmp_e_92122:
	dec	sp
	dec	sp
	ld	-2714(ix), l
	ld	-2713(ix), h
	jp	__xcc_L655
__xcc_L653:
	ld	hl, #1
	ld	-2714(ix), l
	ld	-2713(ix), h
__xcc_L655:
	ld	l, -2714(ix)
	ld	h, -2713(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L650
	jp	__xcc_L651
__xcc_L650:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2716(ix), l
	ld	-2715(ix), h
	ld	l, -2716(ix)
	ld	h, -2715(ix)
	dec	sp
	dec	sp
	ld	-2718(ix), l
	ld	-2717(ix), h
	jp	__xcc_L652
__xcc_L651:
	ld	hl, #1
	ld	-2718(ix), l
	ld	-2717(ix), h
__xcc_L652:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2720(ix), l
	ld	-2719(ix), h
	.globl __mul16
	ld	l, -2720(ix)
	ld	h, -2719(ix)
	push	hl
	ld	l, -2718(ix)
	ld	h, -2717(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2722(ix), l
	ld	-2721(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2726(ix), l
	ld	-2725(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2728(ix), l
	ld	-2727(ix), h
	ld	l, -2726(ix)
	ld	h, -2725(ix)
	push	hl
	ld	l, -2728(ix)
	ld	h, -2727(ix)
	ld	b, l
	pop	hl
__shift_1281:
	ld	a, b
	or	a, a
	jp	z, __sdone_3875
	add	hl, hl
	djnz	__shift_1281
__sdone_3875:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2732(ix), l
	ld	-2731(ix), h
	ld	l, -2732(ix)
	ld	h, -2731(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84850
	ld	hl, #0
	jp	__cmp_e_90179
__cmp_t_84850:
	ld	hl, #1
__cmp_e_90179:
	dec	sp
	dec	sp
	ld	-2734(ix), l
	ld	-2733(ix), h
	ld	l, -2734(ix)
	ld	h, -2733(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76590
	ld	hl, #0
	jp	__cmp_e_2254
__cmp_t_76590:
	ld	hl, #1
__cmp_e_2254:
	dec	sp
	dec	sp
	ld	-2736(ix), l
	ld	-2735(ix), h
	ld	l, -2736(ix)
	ld	h, -2735(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L659
	jp	__xcc_L660
__xcc_L660:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2740(ix), l
	ld	-2739(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2742(ix), l
	ld	-2741(ix), h
	ld	l, -2740(ix)
	ld	h, -2739(ix)
	push	hl
	ld	l, -2742(ix)
	ld	h, -2741(ix)
	ld	b, l
	pop	hl
__shift_5350:
	ld	a, b
	or	a, a
	jp	z, __sdone_1131
	add	hl, hl
	djnz	__shift_5350
__sdone_1131:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2746(ix), l
	ld	-2745(ix), h
	ld	l, -2746(ix)
	ld	h, -2745(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2748(ix), l
	ld	-2747(ix), h
	ld	l, -2748(ix)
	ld	h, -2747(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73813
	ld	hl, #0
	jp	__cmp_e_67857
__cmp_t_73813:
	ld	hl, #1
__cmp_e_67857:
	dec	sp
	dec	sp
	ld	-2750(ix), l
	ld	-2749(ix), h
	ld	l, -2750(ix)
	ld	h, -2749(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81494
	ld	hl, #0
	jp	__cmp_e_99181
__cmp_t_81494:
	ld	hl, #1
__cmp_e_99181:
	dec	sp
	dec	sp
	ld	-2752(ix), l
	ld	-2751(ix), h
	jp	__xcc_L661
__xcc_L659:
	ld	hl, #1
	ld	-2752(ix), l
	ld	-2751(ix), h
__xcc_L661:
	ld	l, -2752(ix)
	ld	h, -2751(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L656
	jp	__xcc_L657
__xcc_L656:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2754(ix), l
	ld	-2753(ix), h
	ld	l, -2754(ix)
	ld	h, -2753(ix)
	dec	sp
	dec	sp
	ld	-2756(ix), l
	ld	-2755(ix), h
	jp	__xcc_L658
__xcc_L657:
	ld	hl, #1
	ld	-2756(ix), l
	ld	-2755(ix), h
__xcc_L658:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2758(ix), l
	ld	-2757(ix), h
	.globl __mul16
	ld	l, -2758(ix)
	ld	h, -2757(ix)
	push	hl
	ld	l, -2756(ix)
	ld	h, -2755(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2760(ix), l
	ld	-2759(ix), h
	ld	l, -2760(ix)
	ld	h, -2759(ix)
	push	hl
	ld	l, -2722(ix)
	ld	h, -2721(ix)
	push	hl
	ld	l, -2696(ix)
	ld	h, -2695(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_662
	dec	sp
	dec	sp
	ld	-2762(ix), l
	ld	-2761(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2766(ix), l
	ld	-2765(ix), h
	ld	l, -2766(ix)
	ld	h, -2765(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_46081
	ld	hl, #0
	jp	__cmp_e_54603
__cmp_t_46081:
	ld	hl, #1
__cmp_e_54603:
	dec	sp
	dec	sp
	ld	-2768(ix), l
	ld	-2767(ix), h
	ld	l, -2768(ix)
	ld	h, -2767(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15720
	ld	hl, #0
	jp	__cmp_e_52433
__cmp_t_15720:
	ld	hl, #1
__cmp_e_52433:
	dec	sp
	dec	sp
	ld	-2770(ix), l
	ld	-2769(ix), h
	ld	l, -2770(ix)
	ld	h, -2769(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L666
	jp	__xcc_L667
__xcc_L667:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2774(ix), l
	ld	-2773(ix), h
	ld	l, -2774(ix)
	ld	h, -2773(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2776(ix), l
	ld	-2775(ix), h
	ld	l, -2776(ix)
	ld	h, -2775(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87982
	ld	hl, #0
	jp	__cmp_e_90181
__cmp_t_87982:
	ld	hl, #1
__cmp_e_90181:
	dec	sp
	dec	sp
	ld	-2778(ix), l
	ld	-2777(ix), h
	ld	l, -2778(ix)
	ld	h, -2777(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97487
	ld	hl, #0
	jp	__cmp_e_59415
__cmp_t_97487:
	ld	hl, #1
__cmp_e_59415:
	dec	sp
	dec	sp
	ld	-2780(ix), l
	ld	-2779(ix), h
	jp	__xcc_L668
__xcc_L666:
	ld	hl, #1
	ld	-2780(ix), l
	ld	-2779(ix), h
__xcc_L668:
	ld	l, -2780(ix)
	ld	h, -2779(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L663
	jp	__xcc_L664
__xcc_L663:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2782(ix), l
	ld	-2781(ix), h
	ld	l, -2782(ix)
	ld	h, -2781(ix)
	dec	sp
	dec	sp
	ld	-2784(ix), l
	ld	-2783(ix), h
	jp	__xcc_L665
__xcc_L664:
	ld	hl, #1
	ld	-2784(ix), l
	ld	-2783(ix), h
__xcc_L665:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2786(ix), l
	ld	-2785(ix), h
	.globl __mul16
	ld	l, -2786(ix)
	ld	h, -2785(ix)
	push	hl
	ld	l, -2784(ix)
	ld	h, -2783(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2788(ix), l
	ld	-2787(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2792(ix), l
	ld	-2791(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2794(ix), l
	ld	-2793(ix), h
	ld	l, -2792(ix)
	ld	h, -2791(ix)
	push	hl
	ld	l, -2794(ix)
	ld	h, -2793(ix)
	ld	b, l
	pop	hl
__shift_9296:
	ld	a, b
	or	a, a
	jp	z, __sdone_8825
	add	hl, hl
	djnz	__shift_9296
__sdone_8825:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2798(ix), l
	ld	-2797(ix), h
	ld	l, -2798(ix)
	ld	h, -2797(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65404
	ld	hl, #0
	jp	__cmp_e_38722
__cmp_t_65404:
	ld	hl, #1
__cmp_e_38722:
	dec	sp
	dec	sp
	ld	-2800(ix), l
	ld	-2799(ix), h
	ld	l, -2800(ix)
	ld	h, -2799(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96892
	ld	hl, #0
	jp	__cmp_e_50551
__cmp_t_96892:
	ld	hl, #1
__cmp_e_50551:
	dec	sp
	dec	sp
	ld	-2802(ix), l
	ld	-2801(ix), h
	ld	l, -2802(ix)
	ld	h, -2801(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L672
	jp	__xcc_L673
__xcc_L673:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2806(ix), l
	ld	-2805(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-2808(ix), l
	ld	-2807(ix), h
	ld	l, -2806(ix)
	ld	h, -2805(ix)
	push	hl
	ld	l, -2808(ix)
	ld	h, -2807(ix)
	ld	b, l
	pop	hl
__shift_297:
	ld	a, b
	or	a, a
	jp	z, __sdone_32
	add	hl, hl
	djnz	__shift_297
__sdone_32:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2812(ix), l
	ld	-2811(ix), h
	ld	l, -2812(ix)
	ld	h, -2811(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2814(ix), l
	ld	-2813(ix), h
	ld	l, -2814(ix)
	ld	h, -2813(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99134
	ld	hl, #0
	jp	__cmp_e_43181
__cmp_t_99134:
	ld	hl, #1
__cmp_e_43181:
	dec	sp
	dec	sp
	ld	-2816(ix), l
	ld	-2815(ix), h
	ld	l, -2816(ix)
	ld	h, -2815(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98506
	ld	hl, #0
	jp	__cmp_e_90415
__cmp_t_98506:
	ld	hl, #1
__cmp_e_90415:
	dec	sp
	dec	sp
	ld	-2818(ix), l
	ld	-2817(ix), h
	jp	__xcc_L674
__xcc_L672:
	ld	hl, #1
	ld	-2818(ix), l
	ld	-2817(ix), h
__xcc_L674:
	ld	l, -2818(ix)
	ld	h, -2817(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L669
	jp	__xcc_L670
__xcc_L669:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2820(ix), l
	ld	-2819(ix), h
	ld	l, -2820(ix)
	ld	h, -2819(ix)
	dec	sp
	dec	sp
	ld	-2822(ix), l
	ld	-2821(ix), h
	jp	__xcc_L671
__xcc_L670:
	ld	hl, #1
	ld	-2822(ix), l
	ld	-2821(ix), h
__xcc_L671:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2824(ix), l
	ld	-2823(ix), h
	.globl __mul16
	ld	l, -2824(ix)
	ld	h, -2823(ix)
	push	hl
	ld	l, -2822(ix)
	ld	h, -2821(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2826(ix), l
	ld	-2825(ix), h
	ld	l, -2826(ix)
	ld	h, -2825(ix)
	push	hl
	ld	l, -2788(ix)
	ld	h, -2787(ix)
	push	hl
	ld	l, -2762(ix)
	ld	h, -2761(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L647:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L646
	jp	__xcc_L648
__xcc_L648:
__xcc_L675:
	ld	hl, #__str_678
	dec	sp
	dec	sp
	ld	-2828(ix), l
	ld	-2827(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2832(ix), l
	ld	-2831(ix), h
	ld	l, -2832(ix)
	ld	h, -2831(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_67057
	ld	hl, #0
	jp	__cmp_e_99708
__cmp_t_67057:
	ld	hl, #1
__cmp_e_99708:
	dec	sp
	dec	sp
	ld	-2834(ix), l
	ld	-2833(ix), h
	ld	l, -2834(ix)
	ld	h, -2833(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80595
	ld	hl, #0
	jp	__cmp_e_59999
__cmp_t_80595:
	ld	hl, #1
__cmp_e_59999:
	dec	sp
	dec	sp
	ld	-2836(ix), l
	ld	-2835(ix), h
	ld	l, -2836(ix)
	ld	h, -2835(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L682
	jp	__xcc_L683
__xcc_L683:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2840(ix), l
	ld	-2839(ix), h
	ld	l, -2840(ix)
	ld	h, -2839(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2842(ix), l
	ld	-2841(ix), h
	ld	l, -2842(ix)
	ld	h, -2841(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1962
	ld	hl, #0
	jp	__cmp_e_12297
__cmp_t_1962:
	ld	hl, #1
__cmp_e_12297:
	dec	sp
	dec	sp
	ld	-2844(ix), l
	ld	-2843(ix), h
	ld	l, -2844(ix)
	ld	h, -2843(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_87483
	ld	hl, #0
	jp	__cmp_e_75776
__cmp_t_87483:
	ld	hl, #1
__cmp_e_75776:
	dec	sp
	dec	sp
	ld	-2846(ix), l
	ld	-2845(ix), h
	jp	__xcc_L684
__xcc_L682:
	ld	hl, #1
	ld	-2846(ix), l
	ld	-2845(ix), h
__xcc_L684:
	ld	l, -2846(ix)
	ld	h, -2845(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L679
	jp	__xcc_L680
__xcc_L679:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2848(ix), l
	ld	-2847(ix), h
	ld	l, -2848(ix)
	ld	h, -2847(ix)
	dec	sp
	dec	sp
	ld	-2850(ix), l
	ld	-2849(ix), h
	jp	__xcc_L681
__xcc_L680:
	ld	hl, #1
	ld	-2850(ix), l
	ld	-2849(ix), h
__xcc_L681:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2852(ix), l
	ld	-2851(ix), h
	.globl __mul16
	ld	l, -2852(ix)
	ld	h, -2851(ix)
	push	hl
	ld	l, -2850(ix)
	ld	h, -2849(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2854(ix), l
	ld	-2853(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2858(ix), l
	ld	-2857(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2862(ix), l
	ld	-2861(ix), h
	ld	l, -2858(ix)
	ld	h, -2857(ix)
	push	hl
	ld	l, -2862(ix)
	ld	h, -2861(ix)
	ld	b, l
	pop	hl
__shift_154:
	ld	a, b
	or	a, a
	jp	z, __sdone_8977
	add	hl, hl
	djnz	__shift_154
__sdone_8977:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2866(ix), l
	ld	-2865(ix), h
	ld	l, -2866(ix)
	ld	h, -2865(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91309
	ld	hl, #0
	jp	__cmp_e_42587
__cmp_t_91309:
	ld	hl, #1
__cmp_e_42587:
	dec	sp
	dec	sp
	ld	-2868(ix), l
	ld	-2867(ix), h
	ld	l, -2868(ix)
	ld	h, -2867(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_39932
	ld	hl, #0
	jp	__cmp_e_23382
__cmp_t_39932:
	ld	hl, #1
__cmp_e_23382:
	dec	sp
	dec	sp
	ld	-2870(ix), l
	ld	-2869(ix), h
	ld	l, -2870(ix)
	ld	h, -2869(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L688
	jp	__xcc_L689
__xcc_L689:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2874(ix), l
	ld	-2873(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2878(ix), l
	ld	-2877(ix), h
	ld	l, -2874(ix)
	ld	h, -2873(ix)
	push	hl
	ld	l, -2878(ix)
	ld	h, -2877(ix)
	ld	b, l
	pop	hl
__shift_5021:
	ld	a, b
	or	a, a
	jp	z, __sdone_4266
	add	hl, hl
	djnz	__shift_5021
__sdone_4266:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2882(ix), l
	ld	-2881(ix), h
	ld	l, -2882(ix)
	ld	h, -2881(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2884(ix), l
	ld	-2883(ix), h
	ld	l, -2884(ix)
	ld	h, -2883(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13563
	ld	hl, #0
	jp	__cmp_e_8860
__cmp_t_13563:
	ld	hl, #1
__cmp_e_8860:
	dec	sp
	dec	sp
	ld	-2886(ix), l
	ld	-2885(ix), h
	ld	l, -2886(ix)
	ld	h, -2885(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3682
	ld	hl, #0
	jp	__cmp_e_9211
__cmp_t_3682:
	ld	hl, #1
__cmp_e_9211:
	dec	sp
	dec	sp
	ld	-2888(ix), l
	ld	-2887(ix), h
	jp	__xcc_L690
__xcc_L688:
	ld	hl, #1
	ld	-2888(ix), l
	ld	-2887(ix), h
__xcc_L690:
	ld	l, -2888(ix)
	ld	h, -2887(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L685
	jp	__xcc_L686
__xcc_L685:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2890(ix), l
	ld	-2889(ix), h
	ld	l, -2890(ix)
	ld	h, -2889(ix)
	dec	sp
	dec	sp
	ld	-2892(ix), l
	ld	-2891(ix), h
	jp	__xcc_L687
__xcc_L686:
	ld	hl, #1
	ld	-2892(ix), l
	ld	-2891(ix), h
__xcc_L687:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2894(ix), l
	ld	-2893(ix), h
	.globl __mul16
	ld	l, -2894(ix)
	ld	h, -2893(ix)
	push	hl
	ld	l, -2892(ix)
	ld	h, -2891(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2896(ix), l
	ld	-2895(ix), h
	ld	l, -2896(ix)
	ld	h, -2895(ix)
	push	hl
	ld	l, -2854(ix)
	ld	h, -2853(ix)
	push	hl
	ld	l, -2828(ix)
	ld	h, -2827(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_691
	dec	sp
	dec	sp
	ld	-2898(ix), l
	ld	-2897(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2902(ix), l
	ld	-2901(ix), h
	ld	l, -2902(ix)
	ld	h, -2901(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_77685
	ld	hl, #0
	jp	__cmp_e_69086
__cmp_t_77685:
	ld	hl, #1
__cmp_e_69086:
	dec	sp
	dec	sp
	ld	-2904(ix), l
	ld	-2903(ix), h
	ld	l, -2904(ix)
	ld	h, -2903(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_64285
	ld	hl, #0
	jp	__cmp_e_90930
__cmp_t_64285:
	ld	hl, #1
__cmp_e_90930:
	dec	sp
	dec	sp
	ld	-2906(ix), l
	ld	-2905(ix), h
	ld	l, -2906(ix)
	ld	h, -2905(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L695
	jp	__xcc_L696
__xcc_L696:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2910(ix), l
	ld	-2909(ix), h
	ld	l, -2910(ix)
	ld	h, -2909(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2912(ix), l
	ld	-2911(ix), h
	ld	l, -2912(ix)
	ld	h, -2911(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35990
	ld	hl, #0
	jp	__cmp_e_94583
__cmp_t_35990:
	ld	hl, #1
__cmp_e_94583:
	dec	sp
	dec	sp
	ld	-2914(ix), l
	ld	-2913(ix), h
	ld	l, -2914(ix)
	ld	h, -2913(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97314
	ld	hl, #0
	jp	__cmp_e_51476
__cmp_t_97314:
	ld	hl, #1
__cmp_e_51476:
	dec	sp
	dec	sp
	ld	-2916(ix), l
	ld	-2915(ix), h
	jp	__xcc_L697
__xcc_L695:
	ld	hl, #1
	ld	-2916(ix), l
	ld	-2915(ix), h
__xcc_L697:
	ld	l, -2916(ix)
	ld	h, -2915(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L692
	jp	__xcc_L693
__xcc_L692:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2918(ix), l
	ld	-2917(ix), h
	ld	l, -2918(ix)
	ld	h, -2917(ix)
	dec	sp
	dec	sp
	ld	-2920(ix), l
	ld	-2919(ix), h
	jp	__xcc_L694
__xcc_L693:
	ld	hl, #1
	ld	-2920(ix), l
	ld	-2919(ix), h
__xcc_L694:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2922(ix), l
	ld	-2921(ix), h
	.globl __mul16
	ld	l, -2922(ix)
	ld	h, -2921(ix)
	push	hl
	ld	l, -2920(ix)
	ld	h, -2919(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2924(ix), l
	ld	-2923(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2928(ix), l
	ld	-2927(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2932(ix), l
	ld	-2931(ix), h
	ld	l, -2928(ix)
	ld	h, -2927(ix)
	push	hl
	ld	l, -2932(ix)
	ld	h, -2931(ix)
	ld	b, l
	pop	hl
__shift_4116:
	ld	a, b
	or	a, a
	jp	z, __sdone_5820
	add	hl, hl
	djnz	__shift_4116
__sdone_5820:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2936(ix), l
	ld	-2935(ix), h
	ld	l, -2936(ix)
	ld	h, -2935(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41892
	ld	hl, #0
	jp	__cmp_e_37525
__cmp_t_41892:
	ld	hl, #1
__cmp_e_37525:
	dec	sp
	dec	sp
	ld	-2938(ix), l
	ld	-2937(ix), h
	ld	l, -2938(ix)
	ld	h, -2937(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95528
	ld	hl, #0
	jp	__cmp_e_38839
__cmp_t_95528:
	ld	hl, #1
__cmp_e_38839:
	dec	sp
	dec	sp
	ld	-2940(ix), l
	ld	-2939(ix), h
	ld	l, -2940(ix)
	ld	h, -2939(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L701
	jp	__xcc_L702
__xcc_L702:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2944(ix), l
	ld	-2943(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2948(ix), l
	ld	-2947(ix), h
	ld	l, -2944(ix)
	ld	h, -2943(ix)
	push	hl
	ld	l, -2948(ix)
	ld	h, -2947(ix)
	ld	b, l
	pop	hl
__shift_7525:
	ld	a, b
	or	a, a
	jp	z, __sdone_7490
	add	hl, hl
	djnz	__shift_7525
__sdone_7490:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2952(ix), l
	ld	-2951(ix), h
	ld	l, -2952(ix)
	ld	h, -2951(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2954(ix), l
	ld	-2953(ix), h
	ld	l, -2954(ix)
	ld	h, -2953(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51136
	ld	hl, #0
	jp	__cmp_e_1360
__cmp_t_51136:
	ld	hl, #1
__cmp_e_1360:
	dec	sp
	dec	sp
	ld	-2956(ix), l
	ld	-2955(ix), h
	ld	l, -2956(ix)
	ld	h, -2955(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89618
	ld	hl, #0
	jp	__cmp_e_47643
__cmp_t_89618:
	ld	hl, #1
__cmp_e_47643:
	dec	sp
	dec	sp
	ld	-2958(ix), l
	ld	-2957(ix), h
	jp	__xcc_L703
__xcc_L701:
	ld	hl, #1
	ld	-2958(ix), l
	ld	-2957(ix), h
__xcc_L703:
	ld	l, -2958(ix)
	ld	h, -2957(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L698
	jp	__xcc_L699
__xcc_L698:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2960(ix), l
	ld	-2959(ix), h
	ld	l, -2960(ix)
	ld	h, -2959(ix)
	dec	sp
	dec	sp
	ld	-2962(ix), l
	ld	-2961(ix), h
	jp	__xcc_L700
__xcc_L699:
	ld	hl, #1
	ld	-2962(ix), l
	ld	-2961(ix), h
__xcc_L700:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2964(ix), l
	ld	-2963(ix), h
	.globl __mul16
	ld	l, -2964(ix)
	ld	h, -2963(ix)
	push	hl
	ld	l, -2962(ix)
	ld	h, -2961(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2966(ix), l
	ld	-2965(ix), h
	ld	l, -2966(ix)
	ld	h, -2965(ix)
	push	hl
	ld	l, -2924(ix)
	ld	h, -2923(ix)
	push	hl
	ld	l, -2898(ix)
	ld	h, -2897(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L676:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L675
	jp	__xcc_L677
__xcc_L677:
__xcc_L704:
	ld	hl, #__str_707
	dec	sp
	dec	sp
	ld	-2968(ix), l
	ld	-2967(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2972(ix), l
	ld	-2971(ix), h
	ld	l, -2972(ix)
	ld	h, -2971(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_70337
	ld	hl, #0
	jp	__cmp_e_80928
__cmp_t_70337:
	ld	hl, #1
__cmp_e_80928:
	dec	sp
	dec	sp
	ld	-2974(ix), l
	ld	-2973(ix), h
	ld	l, -2974(ix)
	ld	h, -2973(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6582
	ld	hl, #0
	jp	__cmp_e_26621
__cmp_t_6582:
	ld	hl, #1
__cmp_e_26621:
	dec	sp
	dec	sp
	ld	-2976(ix), l
	ld	-2975(ix), h
	ld	l, -2976(ix)
	ld	h, -2975(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L711
	jp	__xcc_L712
__xcc_L712:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2980(ix), l
	ld	-2979(ix), h
	ld	l, -2980(ix)
	ld	h, -2979(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2982(ix), l
	ld	-2981(ix), h
	ld	l, -2982(ix)
	ld	h, -2981(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4310
	ld	hl, #0
	jp	__cmp_e_17955
__cmp_t_4310:
	ld	hl, #1
__cmp_e_17955:
	dec	sp
	dec	sp
	ld	-2984(ix), l
	ld	-2983(ix), h
	ld	l, -2984(ix)
	ld	h, -2983(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70888
	ld	hl, #0
	jp	__cmp_e_34225
__cmp_t_70888:
	ld	hl, #1
__cmp_e_34225:
	dec	sp
	dec	sp
	ld	-2986(ix), l
	ld	-2985(ix), h
	jp	__xcc_L713
__xcc_L711:
	ld	hl, #1
	ld	-2986(ix), l
	ld	-2985(ix), h
__xcc_L713:
	ld	l, -2986(ix)
	ld	h, -2985(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L708
	jp	__xcc_L709
__xcc_L708:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-2988(ix), l
	ld	-2987(ix), h
	ld	l, -2988(ix)
	ld	h, -2987(ix)
	dec	sp
	dec	sp
	ld	-2990(ix), l
	ld	-2989(ix), h
	jp	__xcc_L710
__xcc_L709:
	ld	hl, #1
	ld	-2990(ix), l
	ld	-2989(ix), h
__xcc_L710:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-2992(ix), l
	ld	-2991(ix), h
	.globl __mul16
	ld	l, -2992(ix)
	ld	h, -2991(ix)
	push	hl
	ld	l, -2990(ix)
	ld	h, -2989(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2994(ix), l
	ld	-2993(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-2998(ix), l
	ld	-2997(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3006(ix), l
	ld	-3005(ix), h
	ld	l, -2998(ix)
	ld	h, -2997(ix)
	push	hl
	ld	l, -3006(ix)
	ld	h, -3005(ix)
	ld	b, l
	pop	hl
__shift_6815:
	ld	a, b
	or	a, a
	jp	z, __sdone_4570
	add	hl, hl
	djnz	__shift_6815
__sdone_4570:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3014(ix), l
	ld	-3013(ix), h
	ld	l, -3014(ix)
	ld	h, -3013(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_43437
	ld	hl, #0
	jp	__cmp_e_20853
__cmp_t_43437:
	ld	hl, #1
__cmp_e_20853:
	dec	sp
	dec	sp
	ld	-3016(ix), l
	ld	-3015(ix), h
	ld	l, -3016(ix)
	ld	h, -3015(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60008
	ld	hl, #0
	jp	__cmp_e_7722
__cmp_t_60008:
	ld	hl, #1
__cmp_e_7722:
	dec	sp
	dec	sp
	ld	-3018(ix), l
	ld	-3017(ix), h
	ld	l, -3018(ix)
	ld	h, -3017(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L717
	jp	__xcc_L718
__xcc_L718:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3022(ix), l
	ld	-3021(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3030(ix), l
	ld	-3029(ix), h
	ld	l, -3022(ix)
	ld	h, -3021(ix)
	push	hl
	ld	l, -3030(ix)
	ld	h, -3029(ix)
	ld	b, l
	pop	hl
__shift_1783:
	ld	a, b
	or	a, a
	jp	z, __sdone_2350
	add	hl, hl
	djnz	__shift_1783
__sdone_2350:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3038(ix), l
	ld	-3037(ix), h
	ld	l, -3038(ix)
	ld	h, -3037(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3040(ix), l
	ld	-3039(ix), h
	ld	l, -3040(ix)
	ld	h, -3039(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18657
	ld	hl, #0
	jp	__cmp_e_9097
__cmp_t_18657:
	ld	hl, #1
__cmp_e_9097:
	dec	sp
	dec	sp
	ld	-3042(ix), l
	ld	-3041(ix), h
	ld	l, -3042(ix)
	ld	h, -3041(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_63827
	ld	hl, #0
	jp	__cmp_e_89126
__cmp_t_63827:
	ld	hl, #1
__cmp_e_89126:
	dec	sp
	dec	sp
	ld	-3044(ix), l
	ld	-3043(ix), h
	jp	__xcc_L719
__xcc_L717:
	ld	hl, #1
	ld	-3044(ix), l
	ld	-3043(ix), h
__xcc_L719:
	ld	l, -3044(ix)
	ld	h, -3043(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L714
	jp	__xcc_L715
__xcc_L714:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3046(ix), l
	ld	-3045(ix), h
	ld	l, -3046(ix)
	ld	h, -3045(ix)
	dec	sp
	dec	sp
	ld	-3048(ix), l
	ld	-3047(ix), h
	jp	__xcc_L716
__xcc_L715:
	ld	hl, #1
	ld	-3048(ix), l
	ld	-3047(ix), h
__xcc_L716:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3050(ix), l
	ld	-3049(ix), h
	.globl __mul16
	ld	l, -3050(ix)
	ld	h, -3049(ix)
	push	hl
	ld	l, -3048(ix)
	ld	h, -3047(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3052(ix), l
	ld	-3051(ix), h
	ld	l, -3052(ix)
	ld	h, -3051(ix)
	push	hl
	ld	l, -2994(ix)
	ld	h, -2993(ix)
	push	hl
	ld	l, -2968(ix)
	ld	h, -2967(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_720
	dec	sp
	dec	sp
	ld	-3054(ix), l
	ld	-3053(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3058(ix), l
	ld	-3057(ix), h
	ld	l, -3058(ix)
	ld	h, -3057(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_21269
	ld	hl, #0
	jp	__cmp_e_22071
__cmp_t_21269:
	ld	hl, #1
__cmp_e_22071:
	dec	sp
	dec	sp
	ld	-3060(ix), l
	ld	-3059(ix), h
	ld	l, -3060(ix)
	ld	h, -3059(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26651
	ld	hl, #0
	jp	__cmp_e_33149
__cmp_t_26651:
	ld	hl, #1
__cmp_e_33149:
	dec	sp
	dec	sp
	ld	-3062(ix), l
	ld	-3061(ix), h
	ld	l, -3062(ix)
	ld	h, -3061(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L724
	jp	__xcc_L725
__xcc_L725:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3066(ix), l
	ld	-3065(ix), h
	ld	l, -3066(ix)
	ld	h, -3065(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3068(ix), l
	ld	-3067(ix), h
	ld	l, -3068(ix)
	ld	h, -3067(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60910
	ld	hl, #0
	jp	__cmp_e_40528
__cmp_t_60910:
	ld	hl, #1
__cmp_e_40528:
	dec	sp
	dec	sp
	ld	-3070(ix), l
	ld	-3069(ix), h
	ld	l, -3070(ix)
	ld	h, -3069(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30639
	ld	hl, #0
	jp	__cmp_e_28398
__cmp_t_30639:
	ld	hl, #1
__cmp_e_28398:
	dec	sp
	dec	sp
	ld	-3072(ix), l
	ld	-3071(ix), h
	jp	__xcc_L726
__xcc_L724:
	ld	hl, #1
	ld	-3072(ix), l
	ld	-3071(ix), h
__xcc_L726:
	ld	l, -3072(ix)
	ld	h, -3071(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L721
	jp	__xcc_L722
__xcc_L721:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3074(ix), l
	ld	-3073(ix), h
	ld	l, -3074(ix)
	ld	h, -3073(ix)
	dec	sp
	dec	sp
	ld	-3076(ix), l
	ld	-3075(ix), h
	jp	__xcc_L723
__xcc_L722:
	ld	hl, #1
	ld	-3076(ix), l
	ld	-3075(ix), h
__xcc_L723:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-3078(ix), l
	ld	-3077(ix), h
	.globl __mul16
	ld	l, -3078(ix)
	ld	h, -3077(ix)
	push	hl
	ld	l, -3076(ix)
	ld	h, -3075(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3080(ix), l
	ld	-3079(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3084(ix), l
	ld	-3083(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3092(ix), l
	ld	-3091(ix), h
	ld	l, -3084(ix)
	ld	h, -3083(ix)
	push	hl
	ld	l, -3092(ix)
	ld	h, -3091(ix)
	ld	b, l
	pop	hl
__shift_1888:
	ld	a, b
	or	a, a
	jp	z, __sdone_6610
	add	hl, hl
	djnz	__shift_1888
__sdone_6610:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3100(ix), l
	ld	-3099(ix), h
	ld	l, -3100(ix)
	ld	h, -3099(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92393
	ld	hl, #0
	jp	__cmp_e_28577
__cmp_t_92393:
	ld	hl, #1
__cmp_e_28577:
	dec	sp
	dec	sp
	ld	-3102(ix), l
	ld	-3101(ix), h
	ld	l, -3102(ix)
	ld	h, -3101(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33890
	ld	hl, #0
	jp	__cmp_e_98976
__cmp_t_33890:
	ld	hl, #1
__cmp_e_98976:
	dec	sp
	dec	sp
	ld	-3104(ix), l
	ld	-3103(ix), h
	ld	l, -3104(ix)
	ld	h, -3103(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L730
	jp	__xcc_L731
__xcc_L731:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3108(ix), l
	ld	-3107(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3116(ix), l
	ld	-3115(ix), h
	ld	l, -3108(ix)
	ld	h, -3107(ix)
	push	hl
	ld	l, -3116(ix)
	ld	h, -3115(ix)
	ld	b, l
	pop	hl
__shift_5199:
	ld	a, b
	or	a, a
	jp	z, __sdone_4552
	add	hl, hl
	djnz	__shift_5199
__sdone_4552:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3124(ix), l
	ld	-3123(ix), h
	ld	l, -3124(ix)
	ld	h, -3123(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3126(ix), l
	ld	-3125(ix), h
	ld	l, -3126(ix)
	ld	h, -3125(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16931
	ld	hl, #0
	jp	__cmp_e_26087
__cmp_t_16931:
	ld	hl, #1
__cmp_e_26087:
	dec	sp
	dec	sp
	ld	-3128(ix), l
	ld	-3127(ix), h
	ld	l, -3128(ix)
	ld	h, -3127(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88777
	ld	hl, #0
	jp	__cmp_e_60099
__cmp_t_88777:
	ld	hl, #1
__cmp_e_60099:
	dec	sp
	dec	sp
	ld	-3130(ix), l
	ld	-3129(ix), h
	jp	__xcc_L732
__xcc_L730:
	ld	hl, #1
	ld	-3130(ix), l
	ld	-3129(ix), h
__xcc_L732:
	ld	l, -3130(ix)
	ld	h, -3129(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L727
	jp	__xcc_L728
__xcc_L727:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3132(ix), l
	ld	-3131(ix), h
	ld	l, -3132(ix)
	ld	h, -3131(ix)
	dec	sp
	dec	sp
	ld	-3134(ix), l
	ld	-3133(ix), h
	jp	__xcc_L729
__xcc_L728:
	ld	hl, #1
	ld	-3134(ix), l
	ld	-3133(ix), h
__xcc_L729:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3136(ix), l
	ld	-3135(ix), h
	.globl __mul16
	ld	l, -3136(ix)
	ld	h, -3135(ix)
	push	hl
	ld	l, -3134(ix)
	ld	h, -3133(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3138(ix), l
	ld	-3137(ix), h
	ld	l, -3138(ix)
	ld	h, -3137(ix)
	push	hl
	ld	l, -3080(ix)
	ld	h, -3079(ix)
	push	hl
	ld	l, -3054(ix)
	ld	h, -3053(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L705:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L704
	jp	__xcc_L706
__xcc_L706:
__xcc_L615:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L614
	jp	__xcc_L616
__xcc_L616:
__xcc_L493:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L492
	jp	__xcc_L494
__xcc_L494:
__xcc_L733:
__xcc_L736:
__xcc_L739:
	ld	hl, #__str_742
	dec	sp
	dec	sp
	ld	-3140(ix), l
	ld	-3139(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3148(ix), l
	ld	-3147(ix), h
	ld	l, -3148(ix)
	ld	h, -3147(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_657
	ld	hl, #0
	jp	__cmp_e_48566
__cmp_t_657:
	ld	hl, #1
__cmp_e_48566:
	dec	sp
	dec	sp
	ld	-3150(ix), l
	ld	-3149(ix), h
	ld	l, -3150(ix)
	ld	h, -3149(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80952
	ld	hl, #0
	jp	__cmp_e_77017
__cmp_t_80952:
	ld	hl, #1
__cmp_e_77017:
	dec	sp
	dec	sp
	ld	-3152(ix), l
	ld	-3151(ix), h
	ld	l, -3152(ix)
	ld	h, -3151(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L746
	jp	__xcc_L747
__xcc_L747:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3160(ix), l
	ld	-3159(ix), h
	ld	l, -3160(ix)
	ld	h, -3159(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3162(ix), l
	ld	-3161(ix), h
	ld	l, -3162(ix)
	ld	h, -3161(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72641
	ld	hl, #0
	jp	__cmp_e_92735
__cmp_t_72641:
	ld	hl, #1
__cmp_e_92735:
	dec	sp
	dec	sp
	ld	-3164(ix), l
	ld	-3163(ix), h
	ld	l, -3164(ix)
	ld	h, -3163(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89368
	ld	hl, #0
	jp	__cmp_e_91298
__cmp_t_89368:
	ld	hl, #1
__cmp_e_91298:
	dec	sp
	dec	sp
	ld	-3166(ix), l
	ld	-3165(ix), h
	jp	__xcc_L748
__xcc_L746:
	ld	hl, #1
	ld	-3166(ix), l
	ld	-3165(ix), h
__xcc_L748:
	ld	l, -3166(ix)
	ld	h, -3165(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L743
	jp	__xcc_L744
__xcc_L743:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3168(ix), l
	ld	-3167(ix), h
	ld	l, -3168(ix)
	ld	h, -3167(ix)
	dec	sp
	dec	sp
	ld	-3170(ix), l
	ld	-3169(ix), h
	jp	__xcc_L745
__xcc_L744:
	ld	hl, #1
	ld	-3170(ix), l
	ld	-3169(ix), h
__xcc_L745:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3172(ix), l
	ld	-3171(ix), h
	.globl __mul16
	ld	l, -3172(ix)
	ld	h, -3171(ix)
	push	hl
	ld	l, -3170(ix)
	ld	h, -3169(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3174(ix), l
	ld	-3173(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3182(ix), l
	ld	-3181(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3184(ix), l
	ld	-3183(ix), h
	ld	l, -3182(ix)
	ld	h, -3181(ix)
	push	hl
	ld	l, -3184(ix)
	ld	h, -3183(ix)
	ld	b, l
	pop	hl
__shift_8184:
	ld	a, b
	or	a, a
	jp	z, __sdone_3195
	add	hl, hl
	djnz	__shift_8184
__sdone_3195:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3192(ix), l
	ld	-3191(ix), h
	ld	l, -3192(ix)
	ld	h, -3191(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_96776
	ld	hl, #0
	jp	__cmp_e_55805
__cmp_t_96776:
	ld	hl, #1
__cmp_e_55805:
	dec	sp
	dec	sp
	ld	-3194(ix), l
	ld	-3193(ix), h
	ld	l, -3194(ix)
	ld	h, -3193(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75266
	ld	hl, #0
	jp	__cmp_e_23428
__cmp_t_75266:
	ld	hl, #1
__cmp_e_23428:
	dec	sp
	dec	sp
	ld	-3196(ix), l
	ld	-3195(ix), h
	ld	l, -3196(ix)
	ld	h, -3195(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L752
	jp	__xcc_L753
__xcc_L753:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3204(ix), l
	ld	-3203(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3206(ix), l
	ld	-3205(ix), h
	ld	l, -3204(ix)
	ld	h, -3203(ix)
	push	hl
	ld	l, -3206(ix)
	ld	h, -3205(ix)
	ld	b, l
	pop	hl
__shift_8954:
	ld	a, b
	or	a, a
	jp	z, __sdone_2528
	add	hl, hl
	djnz	__shift_8954
__sdone_2528:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3214(ix), l
	ld	-3213(ix), h
	ld	l, -3214(ix)
	ld	h, -3213(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3216(ix), l
	ld	-3215(ix), h
	ld	l, -3216(ix)
	ld	h, -3215(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80308
	ld	hl, #0
	jp	__cmp_e_19593
__cmp_t_80308:
	ld	hl, #1
__cmp_e_19593:
	dec	sp
	dec	sp
	ld	-3218(ix), l
	ld	-3217(ix), h
	ld	l, -3218(ix)
	ld	h, -3217(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97278
	ld	hl, #0
	jp	__cmp_e_22197
__cmp_t_97278:
	ld	hl, #1
__cmp_e_22197:
	dec	sp
	dec	sp
	ld	-3220(ix), l
	ld	-3219(ix), h
	jp	__xcc_L754
__xcc_L752:
	ld	hl, #1
	ld	-3220(ix), l
	ld	-3219(ix), h
__xcc_L754:
	ld	l, -3220(ix)
	ld	h, -3219(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L749
	jp	__xcc_L750
__xcc_L749:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3222(ix), l
	ld	-3221(ix), h
	ld	l, -3222(ix)
	ld	h, -3221(ix)
	dec	sp
	dec	sp
	ld	-3224(ix), l
	ld	-3223(ix), h
	jp	__xcc_L751
__xcc_L750:
	ld	hl, #1
	ld	-3224(ix), l
	ld	-3223(ix), h
__xcc_L751:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3226(ix), l
	ld	-3225(ix), h
	.globl __mul16
	ld	l, -3226(ix)
	ld	h, -3225(ix)
	push	hl
	ld	l, -3224(ix)
	ld	h, -3223(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3228(ix), l
	ld	-3227(ix), h
	ld	l, -3228(ix)
	ld	h, -3227(ix)
	push	hl
	ld	l, -3174(ix)
	ld	h, -3173(ix)
	push	hl
	ld	l, -3140(ix)
	ld	h, -3139(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_755
	dec	sp
	dec	sp
	ld	-3230(ix), l
	ld	-3229(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3238(ix), l
	ld	-3237(ix), h
	ld	l, -3238(ix)
	ld	h, -3237(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72555
	ld	hl, #0
	jp	__cmp_e_89672
__cmp_t_72555:
	ld	hl, #1
__cmp_e_89672:
	dec	sp
	dec	sp
	ld	-3240(ix), l
	ld	-3239(ix), h
	ld	l, -3240(ix)
	ld	h, -3239(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50774
	ld	hl, #0
	jp	__cmp_e_6445
__cmp_t_50774:
	ld	hl, #1
__cmp_e_6445:
	dec	sp
	dec	sp
	ld	-3242(ix), l
	ld	-3241(ix), h
	ld	l, -3242(ix)
	ld	h, -3241(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L759
	jp	__xcc_L760
__xcc_L760:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3250(ix), l
	ld	-3249(ix), h
	ld	l, -3250(ix)
	ld	h, -3249(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3252(ix), l
	ld	-3251(ix), h
	ld	l, -3252(ix)
	ld	h, -3251(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5000
	ld	hl, #0
	jp	__cmp_e_22325
__cmp_t_5000:
	ld	hl, #1
__cmp_e_22325:
	dec	sp
	dec	sp
	ld	-3254(ix), l
	ld	-3253(ix), h
	ld	l, -3254(ix)
	ld	h, -3253(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60997
	ld	hl, #0
	jp	__cmp_e_38283
__cmp_t_60997:
	ld	hl, #1
__cmp_e_38283:
	dec	sp
	dec	sp
	ld	-3256(ix), l
	ld	-3255(ix), h
	jp	__xcc_L761
__xcc_L759:
	ld	hl, #1
	ld	-3256(ix), l
	ld	-3255(ix), h
__xcc_L761:
	ld	l, -3256(ix)
	ld	h, -3255(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L756
	jp	__xcc_L757
__xcc_L756:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3258(ix), l
	ld	-3257(ix), h
	ld	l, -3258(ix)
	ld	h, -3257(ix)
	dec	sp
	dec	sp
	ld	-3260(ix), l
	ld	-3259(ix), h
	jp	__xcc_L758
__xcc_L757:
	ld	hl, #1
	ld	-3260(ix), l
	ld	-3259(ix), h
__xcc_L758:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3262(ix), l
	ld	-3261(ix), h
	.globl __mul16
	ld	l, -3262(ix)
	ld	h, -3261(ix)
	push	hl
	ld	l, -3260(ix)
	ld	h, -3259(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3264(ix), l
	ld	-3263(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3272(ix), l
	ld	-3271(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3274(ix), l
	ld	-3273(ix), h
	ld	l, -3272(ix)
	ld	h, -3271(ix)
	push	hl
	ld	l, -3274(ix)
	ld	h, -3273(ix)
	ld	b, l
	pop	hl
__shift_8412:
	ld	a, b
	or	a, a
	jp	z, __sdone_6127
	add	hl, hl
	djnz	__shift_8412
__sdone_6127:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3282(ix), l
	ld	-3281(ix), h
	ld	l, -3282(ix)
	ld	h, -3281(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98382
	ld	hl, #0
	jp	__cmp_e_65421
__cmp_t_98382:
	ld	hl, #1
__cmp_e_65421:
	dec	sp
	dec	sp
	ld	-3284(ix), l
	ld	-3283(ix), h
	ld	l, -3284(ix)
	ld	h, -3283(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14693
	ld	hl, #0
	jp	__cmp_e_79334
__cmp_t_14693:
	ld	hl, #1
__cmp_e_79334:
	dec	sp
	dec	sp
	ld	-3286(ix), l
	ld	-3285(ix), h
	ld	l, -3286(ix)
	ld	h, -3285(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L765
	jp	__xcc_L766
__xcc_L766:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3294(ix), l
	ld	-3293(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3296(ix), l
	ld	-3295(ix), h
	ld	l, -3294(ix)
	ld	h, -3293(ix)
	push	hl
	ld	l, -3296(ix)
	ld	h, -3295(ix)
	ld	b, l
	pop	hl
__shift_2439:
	ld	a, b
	or	a, a
	jp	z, __sdone_7334
	add	hl, hl
	djnz	__shift_2439
__sdone_7334:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3304(ix), l
	ld	-3303(ix), h
	ld	l, -3304(ix)
	ld	h, -3303(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3306(ix), l
	ld	-3305(ix), h
	ld	l, -3306(ix)
	ld	h, -3305(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_88421
	ld	hl, #0
	jp	__cmp_e_48159
__cmp_t_88421:
	ld	hl, #1
__cmp_e_48159:
	dec	sp
	dec	sp
	ld	-3308(ix), l
	ld	-3307(ix), h
	ld	l, -3308(ix)
	ld	h, -3307(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_94985
	ld	hl, #0
	jp	__cmp_e_22957
__cmp_t_94985:
	ld	hl, #1
__cmp_e_22957:
	dec	sp
	dec	sp
	ld	-3310(ix), l
	ld	-3309(ix), h
	jp	__xcc_L767
__xcc_L765:
	ld	hl, #1
	ld	-3310(ix), l
	ld	-3309(ix), h
__xcc_L767:
	ld	l, -3310(ix)
	ld	h, -3309(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L762
	jp	__xcc_L763
__xcc_L762:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3312(ix), l
	ld	-3311(ix), h
	ld	l, -3312(ix)
	ld	h, -3311(ix)
	dec	sp
	dec	sp
	ld	-3314(ix), l
	ld	-3313(ix), h
	jp	__xcc_L764
__xcc_L763:
	ld	hl, #1
	ld	-3314(ix), l
	ld	-3313(ix), h
__xcc_L764:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3316(ix), l
	ld	-3315(ix), h
	.globl __mul16
	ld	l, -3316(ix)
	ld	h, -3315(ix)
	push	hl
	ld	l, -3314(ix)
	ld	h, -3313(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3318(ix), l
	ld	-3317(ix), h
	ld	l, -3318(ix)
	ld	h, -3317(ix)
	push	hl
	ld	l, -3264(ix)
	ld	h, -3263(ix)
	push	hl
	ld	l, -3230(ix)
	ld	h, -3229(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L740:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L739
	jp	__xcc_L741
__xcc_L741:
__xcc_L768:
	ld	hl, #__str_771
	dec	sp
	dec	sp
	ld	-3320(ix), l
	ld	-3319(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3328(ix), l
	ld	-3327(ix), h
	ld	l, -3328(ix)
	ld	h, -3327(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1354
	ld	hl, #0
	jp	__cmp_e_91761
__cmp_t_1354:
	ld	hl, #1
__cmp_e_91761:
	dec	sp
	dec	sp
	ld	-3330(ix), l
	ld	-3329(ix), h
	ld	l, -3330(ix)
	ld	h, -3329(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_78762
	ld	hl, #0
	jp	__cmp_e_92972
__cmp_t_78762:
	ld	hl, #1
__cmp_e_92972:
	dec	sp
	dec	sp
	ld	-3332(ix), l
	ld	-3331(ix), h
	ld	l, -3332(ix)
	ld	h, -3331(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L775
	jp	__xcc_L776
__xcc_L776:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3340(ix), l
	ld	-3339(ix), h
	ld	l, -3340(ix)
	ld	h, -3339(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3342(ix), l
	ld	-3341(ix), h
	ld	l, -3342(ix)
	ld	h, -3341(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_31541
	ld	hl, #0
	jp	__cmp_e_67716
__cmp_t_31541:
	ld	hl, #1
__cmp_e_67716:
	dec	sp
	dec	sp
	ld	-3344(ix), l
	ld	-3343(ix), h
	ld	l, -3344(ix)
	ld	h, -3343(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_61852
	ld	hl, #0
	jp	__cmp_e_11850
__cmp_t_61852:
	ld	hl, #1
__cmp_e_11850:
	dec	sp
	dec	sp
	ld	-3346(ix), l
	ld	-3345(ix), h
	jp	__xcc_L777
__xcc_L775:
	ld	hl, #1
	ld	-3346(ix), l
	ld	-3345(ix), h
__xcc_L777:
	ld	l, -3346(ix)
	ld	h, -3345(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L772
	jp	__xcc_L773
__xcc_L772:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3348(ix), l
	ld	-3347(ix), h
	ld	l, -3348(ix)
	ld	h, -3347(ix)
	dec	sp
	dec	sp
	ld	-3350(ix), l
	ld	-3349(ix), h
	jp	__xcc_L774
__xcc_L773:
	ld	hl, #1
	ld	-3350(ix), l
	ld	-3349(ix), h
__xcc_L774:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3352(ix), l
	ld	-3351(ix), h
	.globl __mul16
	ld	l, -3352(ix)
	ld	h, -3351(ix)
	push	hl
	ld	l, -3350(ix)
	ld	h, -3349(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3354(ix), l
	ld	-3353(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3362(ix), l
	ld	-3361(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3364(ix), l
	ld	-3363(ix), h
	ld	l, -3362(ix)
	ld	h, -3361(ix)
	push	hl
	ld	l, -3364(ix)
	ld	h, -3363(ix)
	ld	b, l
	pop	hl
__shift_3662:
	ld	a, b
	or	a, a
	jp	z, __sdone_5482
	add	hl, hl
	djnz	__shift_3662
__sdone_5482:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3372(ix), l
	ld	-3371(ix), h
	ld	l, -3372(ix)
	ld	h, -3371(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_50399
	ld	hl, #0
	jp	__cmp_e_76217
__cmp_t_50399:
	ld	hl, #1
__cmp_e_76217:
	dec	sp
	dec	sp
	ld	-3374(ix), l
	ld	-3373(ix), h
	ld	l, -3374(ix)
	ld	h, -3373(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65154
	ld	hl, #0
	jp	__cmp_e_1173
__cmp_t_65154:
	ld	hl, #1
__cmp_e_1173:
	dec	sp
	dec	sp
	ld	-3376(ix), l
	ld	-3375(ix), h
	ld	l, -3376(ix)
	ld	h, -3375(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L781
	jp	__xcc_L782
__xcc_L782:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3384(ix), l
	ld	-3383(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3386(ix), l
	ld	-3385(ix), h
	ld	l, -3384(ix)
	ld	h, -3383(ix)
	push	hl
	ld	l, -3386(ix)
	ld	h, -3385(ix)
	ld	b, l
	pop	hl
__shift_9015:
	ld	a, b
	or	a, a
	jp	z, __sdone_6506
	add	hl, hl
	djnz	__shift_9015
__sdone_6506:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3394(ix), l
	ld	-3393(ix), h
	ld	l, -3394(ix)
	ld	h, -3393(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3396(ix), l
	ld	-3395(ix), h
	ld	l, -3396(ix)
	ld	h, -3395(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39851
	ld	hl, #0
	jp	__cmp_e_76364
__cmp_t_39851:
	ld	hl, #1
__cmp_e_76364:
	dec	sp
	dec	sp
	ld	-3398(ix), l
	ld	-3397(ix), h
	ld	l, -3398(ix)
	ld	h, -3397(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_24790
	ld	hl, #0
	jp	__cmp_e_88263
__cmp_t_24790:
	ld	hl, #1
__cmp_e_88263:
	dec	sp
	dec	sp
	ld	-3400(ix), l
	ld	-3399(ix), h
	jp	__xcc_L783
__xcc_L781:
	ld	hl, #1
	ld	-3400(ix), l
	ld	-3399(ix), h
__xcc_L783:
	ld	l, -3400(ix)
	ld	h, -3399(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L778
	jp	__xcc_L779
__xcc_L778:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3402(ix), l
	ld	-3401(ix), h
	ld	l, -3402(ix)
	ld	h, -3401(ix)
	dec	sp
	dec	sp
	ld	-3404(ix), l
	ld	-3403(ix), h
	jp	__xcc_L780
__xcc_L779:
	ld	hl, #1
	ld	-3404(ix), l
	ld	-3403(ix), h
__xcc_L780:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3406(ix), l
	ld	-3405(ix), h
	.globl __mul16
	ld	l, -3406(ix)
	ld	h, -3405(ix)
	push	hl
	ld	l, -3404(ix)
	ld	h, -3403(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3408(ix), l
	ld	-3407(ix), h
	ld	l, -3408(ix)
	ld	h, -3407(ix)
	push	hl
	ld	l, -3354(ix)
	ld	h, -3353(ix)
	push	hl
	ld	l, -3320(ix)
	ld	h, -3319(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_784
	dec	sp
	dec	sp
	ld	-3410(ix), l
	ld	-3409(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3418(ix), l
	ld	-3417(ix), h
	ld	l, -3418(ix)
	ld	h, -3417(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_42491
	ld	hl, #0
	jp	__cmp_e_23172
__cmp_t_42491:
	ld	hl, #1
__cmp_e_23172:
	dec	sp
	dec	sp
	ld	-3420(ix), l
	ld	-3419(ix), h
	ld	l, -3420(ix)
	ld	h, -3419(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70037
	ld	hl, #0
	jp	__cmp_e_73537
__cmp_t_70037:
	ld	hl, #1
__cmp_e_73537:
	dec	sp
	dec	sp
	ld	-3422(ix), l
	ld	-3421(ix), h
	ld	l, -3422(ix)
	ld	h, -3421(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L788
	jp	__xcc_L789
__xcc_L789:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3430(ix), l
	ld	-3429(ix), h
	ld	l, -3430(ix)
	ld	h, -3429(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3432(ix), l
	ld	-3431(ix), h
	ld	l, -3432(ix)
	ld	h, -3431(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18859
	ld	hl, #0
	jp	__cmp_e_28828
__cmp_t_18859:
	ld	hl, #1
__cmp_e_28828:
	dec	sp
	dec	sp
	ld	-3434(ix), l
	ld	-3433(ix), h
	ld	l, -3434(ix)
	ld	h, -3433(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60871
	ld	hl, #0
	jp	__cmp_e_7280
__cmp_t_60871:
	ld	hl, #1
__cmp_e_7280:
	dec	sp
	dec	sp
	ld	-3436(ix), l
	ld	-3435(ix), h
	jp	__xcc_L790
__xcc_L788:
	ld	hl, #1
	ld	-3436(ix), l
	ld	-3435(ix), h
__xcc_L790:
	ld	l, -3436(ix)
	ld	h, -3435(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L785
	jp	__xcc_L786
__xcc_L785:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3438(ix), l
	ld	-3437(ix), h
	ld	l, -3438(ix)
	ld	h, -3437(ix)
	dec	sp
	dec	sp
	ld	-3440(ix), l
	ld	-3439(ix), h
	jp	__xcc_L787
__xcc_L786:
	ld	hl, #1
	ld	-3440(ix), l
	ld	-3439(ix), h
__xcc_L787:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3442(ix), l
	ld	-3441(ix), h
	.globl __mul16
	ld	l, -3442(ix)
	ld	h, -3441(ix)
	push	hl
	ld	l, -3440(ix)
	ld	h, -3439(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3444(ix), l
	ld	-3443(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3452(ix), l
	ld	-3451(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3454(ix), l
	ld	-3453(ix), h
	ld	l, -3452(ix)
	ld	h, -3451(ix)
	push	hl
	ld	l, -3454(ix)
	ld	h, -3453(ix)
	ld	b, l
	pop	hl
__shift_6987:
	ld	a, b
	or	a, a
	jp	z, __sdone_5856
	add	hl, hl
	djnz	__shift_6987
__sdone_5856:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3462(ix), l
	ld	-3461(ix), h
	ld	l, -3462(ix)
	ld	h, -3461(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_46590
	ld	hl, #0
	jp	__cmp_e_78341
__cmp_t_46590:
	ld	hl, #1
__cmp_e_78341:
	dec	sp
	dec	sp
	ld	-3464(ix), l
	ld	-3463(ix), h
	ld	l, -3464(ix)
	ld	h, -3463(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_63970
	ld	hl, #0
	jp	__cmp_e_25352
__cmp_t_63970:
	ld	hl, #1
__cmp_e_25352:
	dec	sp
	dec	sp
	ld	-3466(ix), l
	ld	-3465(ix), h
	ld	l, -3466(ix)
	ld	h, -3465(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L794
	jp	__xcc_L795
__xcc_L795:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3474(ix), l
	ld	-3473(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3476(ix), l
	ld	-3475(ix), h
	ld	l, -3474(ix)
	ld	h, -3473(ix)
	push	hl
	ld	l, -3476(ix)
	ld	h, -3475(ix)
	ld	b, l
	pop	hl
__shift_7665:
	ld	a, b
	or	a, a
	jp	z, __sdone_5511
	add	hl, hl
	djnz	__shift_7665
__sdone_5511:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3484(ix), l
	ld	-3483(ix), h
	ld	l, -3484(ix)
	ld	h, -3483(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3486(ix), l
	ld	-3485(ix), h
	ld	l, -3486(ix)
	ld	h, -3485(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_93069
	ld	hl, #0
	jp	__cmp_e_49517
__cmp_t_93069:
	ld	hl, #1
__cmp_e_49517:
	dec	sp
	dec	sp
	ld	-3488(ix), l
	ld	-3487(ix), h
	ld	l, -3488(ix)
	ld	h, -3487(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7361
	ld	hl, #0
	jp	__cmp_e_13083
__cmp_t_7361:
	ld	hl, #1
__cmp_e_13083:
	dec	sp
	dec	sp
	ld	-3490(ix), l
	ld	-3489(ix), h
	jp	__xcc_L796
__xcc_L794:
	ld	hl, #1
	ld	-3490(ix), l
	ld	-3489(ix), h
__xcc_L796:
	ld	l, -3490(ix)
	ld	h, -3489(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L791
	jp	__xcc_L792
__xcc_L791:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3492(ix), l
	ld	-3491(ix), h
	ld	l, -3492(ix)
	ld	h, -3491(ix)
	dec	sp
	dec	sp
	ld	-3494(ix), l
	ld	-3493(ix), h
	jp	__xcc_L793
__xcc_L792:
	ld	hl, #1
	ld	-3494(ix), l
	ld	-3493(ix), h
__xcc_L793:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3496(ix), l
	ld	-3495(ix), h
	.globl __mul16
	ld	l, -3496(ix)
	ld	h, -3495(ix)
	push	hl
	ld	l, -3494(ix)
	ld	h, -3493(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3498(ix), l
	ld	-3497(ix), h
	ld	l, -3498(ix)
	ld	h, -3497(ix)
	push	hl
	ld	l, -3444(ix)
	ld	h, -3443(ix)
	push	hl
	ld	l, -3410(ix)
	ld	h, -3409(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L769:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L768
	jp	__xcc_L770
__xcc_L770:
__xcc_L797:
	ld	hl, #__str_800
	dec	sp
	dec	sp
	ld	-3500(ix), l
	ld	-3499(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3508(ix), l
	ld	-3507(ix), h
	ld	l, -3508(ix)
	ld	h, -3507(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41351
	ld	hl, #0
	jp	__cmp_e_74112
__cmp_t_41351:
	ld	hl, #1
__cmp_e_74112:
	dec	sp
	dec	sp
	ld	-3510(ix), l
	ld	-3509(ix), h
	ld	l, -3510(ix)
	ld	h, -3509(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89300
	ld	hl, #0
	jp	__cmp_e_6506
__cmp_t_89300:
	ld	hl, #1
__cmp_e_6506:
	dec	sp
	dec	sp
	ld	-3512(ix), l
	ld	-3511(ix), h
	ld	l, -3512(ix)
	ld	h, -3511(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L804
	jp	__xcc_L805
__xcc_L805:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3520(ix), l
	ld	-3519(ix), h
	ld	l, -3520(ix)
	ld	h, -3519(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3522(ix), l
	ld	-3521(ix), h
	ld	l, -3522(ix)
	ld	h, -3521(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91638
	ld	hl, #0
	jp	__cmp_e_4667
__cmp_t_91638:
	ld	hl, #1
__cmp_e_4667:
	dec	sp
	dec	sp
	ld	-3524(ix), l
	ld	-3523(ix), h
	ld	l, -3524(ix)
	ld	h, -3523(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_9364
	ld	hl, #0
	jp	__cmp_e_31489
__cmp_t_9364:
	ld	hl, #1
__cmp_e_31489:
	dec	sp
	dec	sp
	ld	-3526(ix), l
	ld	-3525(ix), h
	jp	__xcc_L806
__xcc_L804:
	ld	hl, #1
	ld	-3526(ix), l
	ld	-3525(ix), h
__xcc_L806:
	ld	l, -3526(ix)
	ld	h, -3525(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L801
	jp	__xcc_L802
__xcc_L801:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3528(ix), l
	ld	-3527(ix), h
	ld	l, -3528(ix)
	ld	h, -3527(ix)
	dec	sp
	dec	sp
	ld	-3530(ix), l
	ld	-3529(ix), h
	jp	__xcc_L803
__xcc_L802:
	ld	hl, #1
	ld	-3530(ix), l
	ld	-3529(ix), h
__xcc_L803:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3532(ix), l
	ld	-3531(ix), h
	.globl __mul16
	ld	l, -3532(ix)
	ld	h, -3531(ix)
	push	hl
	ld	l, -3530(ix)
	ld	h, -3529(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3534(ix), l
	ld	-3533(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3542(ix), l
	ld	-3541(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3546(ix), l
	ld	-3545(ix), h
	ld	l, -3542(ix)
	ld	h, -3541(ix)
	push	hl
	ld	l, -3546(ix)
	ld	h, -3545(ix)
	ld	b, l
	pop	hl
__shift_1032:
	ld	a, b
	or	a, a
	jp	z, __sdone_4154
	add	hl, hl
	djnz	__shift_1032
__sdone_4154:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3554(ix), l
	ld	-3553(ix), h
	ld	l, -3554(ix)
	ld	h, -3553(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_36104
	ld	hl, #0
	jp	__cmp_e_39875
__cmp_t_36104:
	ld	hl, #1
__cmp_e_39875:
	dec	sp
	dec	sp
	ld	-3556(ix), l
	ld	-3555(ix), h
	ld	l, -3556(ix)
	ld	h, -3555(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73679
	ld	hl, #0
	jp	__cmp_e_6141
__cmp_t_73679:
	ld	hl, #1
__cmp_e_6141:
	dec	sp
	dec	sp
	ld	-3558(ix), l
	ld	-3557(ix), h
	ld	l, -3558(ix)
	ld	h, -3557(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L810
	jp	__xcc_L811
__xcc_L811:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3566(ix), l
	ld	-3565(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3570(ix), l
	ld	-3569(ix), h
	ld	l, -3566(ix)
	ld	h, -3565(ix)
	push	hl
	ld	l, -3570(ix)
	ld	h, -3569(ix)
	ld	b, l
	pop	hl
__shift_3412:
	ld	a, b
	or	a, a
	jp	z, __sdone_2538
	add	hl, hl
	djnz	__shift_3412
__sdone_2538:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3578(ix), l
	ld	-3577(ix), h
	ld	l, -3578(ix)
	ld	h, -3577(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3580(ix), l
	ld	-3579(ix), h
	ld	l, -3580(ix)
	ld	h, -3579(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_34969
	ld	hl, #0
	jp	__cmp_e_90636
__cmp_t_34969:
	ld	hl, #1
__cmp_e_90636:
	dec	sp
	dec	sp
	ld	-3582(ix), l
	ld	-3581(ix), h
	ld	l, -3582(ix)
	ld	h, -3581(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16170
	ld	hl, #0
	jp	__cmp_e_11956
__cmp_t_16170:
	ld	hl, #1
__cmp_e_11956:
	dec	sp
	dec	sp
	ld	-3584(ix), l
	ld	-3583(ix), h
	jp	__xcc_L812
__xcc_L810:
	ld	hl, #1
	ld	-3584(ix), l
	ld	-3583(ix), h
__xcc_L812:
	ld	l, -3584(ix)
	ld	h, -3583(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L807
	jp	__xcc_L808
__xcc_L807:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3586(ix), l
	ld	-3585(ix), h
	ld	l, -3586(ix)
	ld	h, -3585(ix)
	dec	sp
	dec	sp
	ld	-3588(ix), l
	ld	-3587(ix), h
	jp	__xcc_L809
__xcc_L808:
	ld	hl, #1
	ld	-3588(ix), l
	ld	-3587(ix), h
__xcc_L809:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3590(ix), l
	ld	-3589(ix), h
	.globl __mul16
	ld	l, -3590(ix)
	ld	h, -3589(ix)
	push	hl
	ld	l, -3588(ix)
	ld	h, -3587(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3592(ix), l
	ld	-3591(ix), h
	ld	l, -3592(ix)
	ld	h, -3591(ix)
	push	hl
	ld	l, -3534(ix)
	ld	h, -3533(ix)
	push	hl
	ld	l, -3500(ix)
	ld	h, -3499(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_813
	dec	sp
	dec	sp
	ld	-3594(ix), l
	ld	-3593(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3602(ix), l
	ld	-3601(ix), h
	ld	l, -3602(ix)
	ld	h, -3601(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_62844
	ld	hl, #0
	jp	__cmp_e_62760
__cmp_t_62844:
	ld	hl, #1
__cmp_e_62760:
	dec	sp
	dec	sp
	ld	-3604(ix), l
	ld	-3603(ix), h
	ld	l, -3604(ix)
	ld	h, -3603(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6649
	ld	hl, #0
	jp	__cmp_e_26814
__cmp_t_6649:
	ld	hl, #1
__cmp_e_26814:
	dec	sp
	dec	sp
	ld	-3606(ix), l
	ld	-3605(ix), h
	ld	l, -3606(ix)
	ld	h, -3605(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L817
	jp	__xcc_L818
__xcc_L818:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3614(ix), l
	ld	-3613(ix), h
	ld	l, -3614(ix)
	ld	h, -3613(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3616(ix), l
	ld	-3615(ix), h
	ld	l, -3616(ix)
	ld	h, -3615(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4465
	ld	hl, #0
	jp	__cmp_e_94314
__cmp_t_4465:
	ld	hl, #1
__cmp_e_94314:
	dec	sp
	dec	sp
	ld	-3618(ix), l
	ld	-3617(ix), h
	ld	l, -3618(ix)
	ld	h, -3617(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22326
	ld	hl, #0
	jp	__cmp_e_13886
__cmp_t_22326:
	ld	hl, #1
__cmp_e_13886:
	dec	sp
	dec	sp
	ld	-3620(ix), l
	ld	-3619(ix), h
	jp	__xcc_L819
__xcc_L817:
	ld	hl, #1
	ld	-3620(ix), l
	ld	-3619(ix), h
__xcc_L819:
	ld	l, -3620(ix)
	ld	h, -3619(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L814
	jp	__xcc_L815
__xcc_L814:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3622(ix), l
	ld	-3621(ix), h
	ld	l, -3622(ix)
	ld	h, -3621(ix)
	dec	sp
	dec	sp
	ld	-3624(ix), l
	ld	-3623(ix), h
	jp	__xcc_L816
__xcc_L815:
	ld	hl, #1
	ld	-3624(ix), l
	ld	-3623(ix), h
__xcc_L816:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3626(ix), l
	ld	-3625(ix), h
	.globl __mul16
	ld	l, -3626(ix)
	ld	h, -3625(ix)
	push	hl
	ld	l, -3624(ix)
	ld	h, -3623(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3628(ix), l
	ld	-3627(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3636(ix), l
	ld	-3635(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3640(ix), l
	ld	-3639(ix), h
	ld	l, -3636(ix)
	ld	h, -3635(ix)
	push	hl
	ld	l, -3640(ix)
	ld	h, -3639(ix)
	ld	b, l
	pop	hl
__shift_183:
	ld	a, b
	or	a, a
	jp	z, __sdone_6039
	add	hl, hl
	djnz	__shift_183
__sdone_6039:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3648(ix), l
	ld	-3647(ix), h
	ld	l, -3648(ix)
	ld	h, -3647(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_26969
	ld	hl, #0
	jp	__cmp_e_1535
__cmp_t_26969:
	ld	hl, #1
__cmp_e_1535:
	dec	sp
	dec	sp
	ld	-3650(ix), l
	ld	-3649(ix), h
	ld	l, -3650(ix)
	ld	h, -3649(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_20152
	ld	hl, #0
	jp	__cmp_e_32621
__cmp_t_20152:
	ld	hl, #1
__cmp_e_32621:
	dec	sp
	dec	sp
	ld	-3652(ix), l
	ld	-3651(ix), h
	ld	l, -3652(ix)
	ld	h, -3651(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L823
	jp	__xcc_L824
__xcc_L824:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3660(ix), l
	ld	-3659(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3664(ix), l
	ld	-3663(ix), h
	ld	l, -3660(ix)
	ld	h, -3659(ix)
	push	hl
	ld	l, -3664(ix)
	ld	h, -3663(ix)
	ld	b, l
	pop	hl
__shift_4393:
	ld	a, b
	or	a, a
	jp	z, __sdone_1790
	add	hl, hl
	djnz	__shift_4393
__sdone_1790:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3672(ix), l
	ld	-3671(ix), h
	ld	l, -3672(ix)
	ld	h, -3671(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3674(ix), l
	ld	-3673(ix), h
	ld	l, -3674(ix)
	ld	h, -3673(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37289
	ld	hl, #0
	jp	__cmp_e_50109
__cmp_t_37289:
	ld	hl, #1
__cmp_e_50109:
	dec	sp
	dec	sp
	ld	-3676(ix), l
	ld	-3675(ix), h
	ld	l, -3676(ix)
	ld	h, -3675(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_59631
	ld	hl, #0
	jp	__cmp_e_34673
__cmp_t_59631:
	ld	hl, #1
__cmp_e_34673:
	dec	sp
	dec	sp
	ld	-3678(ix), l
	ld	-3677(ix), h
	jp	__xcc_L825
__xcc_L823:
	ld	hl, #1
	ld	-3678(ix), l
	ld	-3677(ix), h
__xcc_L825:
	ld	l, -3678(ix)
	ld	h, -3677(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L820
	jp	__xcc_L821
__xcc_L820:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3680(ix), l
	ld	-3679(ix), h
	ld	l, -3680(ix)
	ld	h, -3679(ix)
	dec	sp
	dec	sp
	ld	-3682(ix), l
	ld	-3681(ix), h
	jp	__xcc_L822
__xcc_L821:
	ld	hl, #1
	ld	-3682(ix), l
	ld	-3681(ix), h
__xcc_L822:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3684(ix), l
	ld	-3683(ix), h
	.globl __mul16
	ld	l, -3684(ix)
	ld	h, -3683(ix)
	push	hl
	ld	l, -3682(ix)
	ld	h, -3681(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3686(ix), l
	ld	-3685(ix), h
	ld	l, -3686(ix)
	ld	h, -3685(ix)
	push	hl
	ld	l, -3628(ix)
	ld	h, -3627(ix)
	push	hl
	ld	l, -3594(ix)
	ld	h, -3593(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L798:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L797
	jp	__xcc_L799
__xcc_L799:
__xcc_L826:
	ld	hl, #__str_829
	dec	sp
	dec	sp
	ld	-3688(ix), l
	ld	-3687(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3696(ix), l
	ld	-3695(ix), h
	ld	l, -3696(ix)
	ld	h, -3695(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84264
	ld	hl, #0
	jp	__cmp_e_95735
__cmp_t_84264:
	ld	hl, #1
__cmp_e_95735:
	dec	sp
	dec	sp
	ld	-3698(ix), l
	ld	-3697(ix), h
	ld	l, -3698(ix)
	ld	h, -3697(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74548
	ld	hl, #0
	jp	__cmp_e_74295
__cmp_t_74548:
	ld	hl, #1
__cmp_e_74295:
	dec	sp
	dec	sp
	ld	-3700(ix), l
	ld	-3699(ix), h
	ld	l, -3700(ix)
	ld	h, -3699(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L833
	jp	__xcc_L834
__xcc_L834:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3708(ix), l
	ld	-3707(ix), h
	ld	l, -3708(ix)
	ld	h, -3707(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3710(ix), l
	ld	-3709(ix), h
	ld	l, -3710(ix)
	ld	h, -3709(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1877
	ld	hl, #0
	jp	__cmp_e_4313
__cmp_t_1877:
	ld	hl, #1
__cmp_e_4313:
	dec	sp
	dec	sp
	ld	-3712(ix), l
	ld	-3711(ix), h
	ld	l, -3712(ix)
	ld	h, -3711(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66833
	ld	hl, #0
	jp	__cmp_e_53198
__cmp_t_66833:
	ld	hl, #1
__cmp_e_53198:
	dec	sp
	dec	sp
	ld	-3714(ix), l
	ld	-3713(ix), h
	jp	__xcc_L835
__xcc_L833:
	ld	hl, #1
	ld	-3714(ix), l
	ld	-3713(ix), h
__xcc_L835:
	ld	l, -3714(ix)
	ld	h, -3713(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L830
	jp	__xcc_L831
__xcc_L830:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3716(ix), l
	ld	-3715(ix), h
	ld	l, -3716(ix)
	ld	h, -3715(ix)
	dec	sp
	dec	sp
	ld	-3718(ix), l
	ld	-3717(ix), h
	jp	__xcc_L832
__xcc_L831:
	ld	hl, #1
	ld	-3718(ix), l
	ld	-3717(ix), h
__xcc_L832:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3720(ix), l
	ld	-3719(ix), h
	.globl __mul16
	ld	l, -3720(ix)
	ld	h, -3719(ix)
	push	hl
	ld	l, -3718(ix)
	ld	h, -3717(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3722(ix), l
	ld	-3721(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3730(ix), l
	ld	-3729(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3738(ix), l
	ld	-3737(ix), h
	ld	l, -3730(ix)
	ld	h, -3729(ix)
	push	hl
	ld	l, -3738(ix)
	ld	h, -3737(ix)
	ld	b, l
	pop	hl
__shift_4949:
	ld	a, b
	or	a, a
	jp	z, __sdone_9355
	add	hl, hl
	djnz	__shift_4949
__sdone_9355:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3746(ix), l
	ld	-3745(ix), h
	ld	l, -3746(ix)
	ld	h, -3745(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65155
	ld	hl, #0
	jp	__cmp_e_57793
__cmp_t_65155:
	ld	hl, #1
__cmp_e_57793:
	dec	sp
	dec	sp
	ld	-3748(ix), l
	ld	-3747(ix), h
	ld	l, -3748(ix)
	ld	h, -3747(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_78468
	ld	hl, #0
	jp	__cmp_e_88156
__cmp_t_78468:
	ld	hl, #1
__cmp_e_88156:
	dec	sp
	dec	sp
	ld	-3750(ix), l
	ld	-3749(ix), h
	ld	l, -3750(ix)
	ld	h, -3749(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L839
	jp	__xcc_L840
__xcc_L840:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3758(ix), l
	ld	-3757(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3766(ix), l
	ld	-3765(ix), h
	ld	l, -3758(ix)
	ld	h, -3757(ix)
	push	hl
	ld	l, -3766(ix)
	ld	h, -3765(ix)
	ld	b, l
	pop	hl
__shift_960:
	ld	a, b
	or	a, a
	jp	z, __sdone_2933
	add	hl, hl
	djnz	__shift_960
__sdone_2933:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3774(ix), l
	ld	-3773(ix), h
	ld	l, -3774(ix)
	ld	h, -3773(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3776(ix), l
	ld	-3775(ix), h
	ld	l, -3776(ix)
	ld	h, -3775(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98823
	ld	hl, #0
	jp	__cmp_e_23286
__cmp_t_98823:
	ld	hl, #1
__cmp_e_23286:
	dec	sp
	dec	sp
	ld	-3778(ix), l
	ld	-3777(ix), h
	ld	l, -3778(ix)
	ld	h, -3777(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13171
	ld	hl, #0
	jp	__cmp_e_75358
__cmp_t_13171:
	ld	hl, #1
__cmp_e_75358:
	dec	sp
	dec	sp
	ld	-3780(ix), l
	ld	-3779(ix), h
	jp	__xcc_L841
__xcc_L839:
	ld	hl, #1
	ld	-3780(ix), l
	ld	-3779(ix), h
__xcc_L841:
	ld	l, -3780(ix)
	ld	h, -3779(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L836
	jp	__xcc_L837
__xcc_L836:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3782(ix), l
	ld	-3781(ix), h
	ld	l, -3782(ix)
	ld	h, -3781(ix)
	dec	sp
	dec	sp
	ld	-3784(ix), l
	ld	-3783(ix), h
	jp	__xcc_L838
__xcc_L837:
	ld	hl, #1
	ld	-3784(ix), l
	ld	-3783(ix), h
__xcc_L838:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3786(ix), l
	ld	-3785(ix), h
	.globl __mul16
	ld	l, -3786(ix)
	ld	h, -3785(ix)
	push	hl
	ld	l, -3784(ix)
	ld	h, -3783(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3788(ix), l
	ld	-3787(ix), h
	ld	l, -3788(ix)
	ld	h, -3787(ix)
	push	hl
	ld	l, -3722(ix)
	ld	h, -3721(ix)
	push	hl
	ld	l, -3688(ix)
	ld	h, -3687(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_842
	dec	sp
	dec	sp
	ld	-3790(ix), l
	ld	-3789(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3798(ix), l
	ld	-3797(ix), h
	ld	l, -3798(ix)
	ld	h, -3797(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85677
	ld	hl, #0
	jp	__cmp_e_40140
__cmp_t_85677:
	ld	hl, #1
__cmp_e_40140:
	dec	sp
	dec	sp
	ld	-3800(ix), l
	ld	-3799(ix), h
	ld	l, -3800(ix)
	ld	h, -3799(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93245
	ld	hl, #0
	jp	__cmp_e_22181
__cmp_t_93245:
	ld	hl, #1
__cmp_e_22181:
	dec	sp
	dec	sp
	ld	-3802(ix), l
	ld	-3801(ix), h
	ld	l, -3802(ix)
	ld	h, -3801(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L846
	jp	__xcc_L847
__xcc_L847:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3810(ix), l
	ld	-3809(ix), h
	ld	l, -3810(ix)
	ld	h, -3809(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3812(ix), l
	ld	-3811(ix), h
	ld	l, -3812(ix)
	ld	h, -3811(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72761
	ld	hl, #0
	jp	__cmp_e_33990
__cmp_t_72761:
	ld	hl, #1
__cmp_e_33990:
	dec	sp
	dec	sp
	ld	-3814(ix), l
	ld	-3813(ix), h
	ld	l, -3814(ix)
	ld	h, -3813(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50323
	ld	hl, #0
	jp	__cmp_e_10050
__cmp_t_50323:
	ld	hl, #1
__cmp_e_10050:
	dec	sp
	dec	sp
	ld	-3816(ix), l
	ld	-3815(ix), h
	jp	__xcc_L848
__xcc_L846:
	ld	hl, #1
	ld	-3816(ix), l
	ld	-3815(ix), h
__xcc_L848:
	ld	l, -3816(ix)
	ld	h, -3815(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L843
	jp	__xcc_L844
__xcc_L843:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3818(ix), l
	ld	-3817(ix), h
	ld	l, -3818(ix)
	ld	h, -3817(ix)
	dec	sp
	dec	sp
	ld	-3820(ix), l
	ld	-3819(ix), h
	jp	__xcc_L845
__xcc_L844:
	ld	hl, #1
	ld	-3820(ix), l
	ld	-3819(ix), h
__xcc_L845:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3822(ix), l
	ld	-3821(ix), h
	.globl __mul16
	ld	l, -3822(ix)
	ld	h, -3821(ix)
	push	hl
	ld	l, -3820(ix)
	ld	h, -3819(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3824(ix), l
	ld	-3823(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3832(ix), l
	ld	-3831(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3840(ix), l
	ld	-3839(ix), h
	ld	l, -3832(ix)
	ld	h, -3831(ix)
	push	hl
	ld	l, -3840(ix)
	ld	h, -3839(ix)
	ld	b, l
	pop	hl
__shift_4100:
	ld	a, b
	or	a, a
	jp	z, __sdone_9954
	add	hl, hl
	djnz	__shift_4100
__sdone_9954:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3848(ix), l
	ld	-3847(ix), h
	ld	l, -3848(ix)
	ld	h, -3847(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_61075
	ld	hl, #0
	jp	__cmp_e_68364
__cmp_t_61075:
	ld	hl, #1
__cmp_e_68364:
	dec	sp
	dec	sp
	ld	-3850(ix), l
	ld	-3849(ix), h
	ld	l, -3850(ix)
	ld	h, -3849(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22042
	ld	hl, #0
	jp	__cmp_e_35624
__cmp_t_22042:
	ld	hl, #1
__cmp_e_35624:
	dec	sp
	dec	sp
	ld	-3852(ix), l
	ld	-3851(ix), h
	ld	l, -3852(ix)
	ld	h, -3851(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L852
	jp	__xcc_L853
__xcc_L853:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3860(ix), l
	ld	-3859(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3868(ix), l
	ld	-3867(ix), h
	ld	l, -3860(ix)
	ld	h, -3859(ix)
	push	hl
	ld	l, -3868(ix)
	ld	h, -3867(ix)
	ld	b, l
	pop	hl
__shift_2659:
	ld	a, b
	or	a, a
	jp	z, __sdone_3919
	add	hl, hl
	djnz	__shift_2659
__sdone_3919:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3876(ix), l
	ld	-3875(ix), h
	ld	l, -3876(ix)
	ld	h, -3875(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3878(ix), l
	ld	-3877(ix), h
	ld	l, -3878(ix)
	ld	h, -3877(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_56289
	ld	hl, #0
	jp	__cmp_e_25844
__cmp_t_56289:
	ld	hl, #1
__cmp_e_25844:
	dec	sp
	dec	sp
	ld	-3880(ix), l
	ld	-3879(ix), h
	ld	l, -3880(ix)
	ld	h, -3879(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93469
	ld	hl, #0
	jp	__cmp_e_51238
__cmp_t_93469:
	ld	hl, #1
__cmp_e_51238:
	dec	sp
	dec	sp
	ld	-3882(ix), l
	ld	-3881(ix), h
	jp	__xcc_L854
__xcc_L852:
	ld	hl, #1
	ld	-3882(ix), l
	ld	-3881(ix), h
__xcc_L854:
	ld	l, -3882(ix)
	ld	h, -3881(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L849
	jp	__xcc_L850
__xcc_L849:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3884(ix), l
	ld	-3883(ix), h
	ld	l, -3884(ix)
	ld	h, -3883(ix)
	dec	sp
	dec	sp
	ld	-3886(ix), l
	ld	-3885(ix), h
	jp	__xcc_L851
__xcc_L850:
	ld	hl, #1
	ld	-3886(ix), l
	ld	-3885(ix), h
__xcc_L851:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3888(ix), l
	ld	-3887(ix), h
	.globl __mul16
	ld	l, -3888(ix)
	ld	h, -3887(ix)
	push	hl
	ld	l, -3886(ix)
	ld	h, -3885(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3890(ix), l
	ld	-3889(ix), h
	ld	l, -3890(ix)
	ld	h, -3889(ix)
	push	hl
	ld	l, -3824(ix)
	ld	h, -3823(ix)
	push	hl
	ld	l, -3790(ix)
	ld	h, -3789(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L827:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L826
	jp	__xcc_L828
__xcc_L828:
__xcc_L737:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L736
	jp	__xcc_L738
__xcc_L738:
__xcc_L855:
__xcc_L858:
	ld	hl, #__str_861
	dec	sp
	dec	sp
	ld	-3892(ix), l
	ld	-3891(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3900(ix), l
	ld	-3899(ix), h
	ld	l, -3900(ix)
	ld	h, -3899(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41551
	ld	hl, #0
	jp	__cmp_e_74976
__cmp_t_41551:
	ld	hl, #1
__cmp_e_74976:
	dec	sp
	dec	sp
	ld	-3902(ix), l
	ld	-3901(ix), h
	ld	l, -3902(ix)
	ld	h, -3901(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25383
	ld	hl, #0
	jp	__cmp_e_20019
__cmp_t_25383:
	ld	hl, #1
__cmp_e_20019:
	dec	sp
	dec	sp
	ld	-3904(ix), l
	ld	-3903(ix), h
	ld	l, -3904(ix)
	ld	h, -3903(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L865
	jp	__xcc_L866
__xcc_L866:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3912(ix), l
	ld	-3911(ix), h
	ld	l, -3912(ix)
	ld	h, -3911(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3914(ix), l
	ld	-3913(ix), h
	ld	l, -3914(ix)
	ld	h, -3913(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63133
	ld	hl, #0
	jp	__cmp_e_26343
__cmp_t_63133:
	ld	hl, #1
__cmp_e_26343:
	dec	sp
	dec	sp
	ld	-3916(ix), l
	ld	-3915(ix), h
	ld	l, -3916(ix)
	ld	h, -3915(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19304
	ld	hl, #0
	jp	__cmp_e_61956
__cmp_t_19304:
	ld	hl, #1
__cmp_e_61956:
	dec	sp
	dec	sp
	ld	-3918(ix), l
	ld	-3917(ix), h
	jp	__xcc_L867
__xcc_L865:
	ld	hl, #1
	ld	-3918(ix), l
	ld	-3917(ix), h
__xcc_L867:
	ld	l, -3918(ix)
	ld	h, -3917(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L862
	jp	__xcc_L863
__xcc_L862:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3920(ix), l
	ld	-3919(ix), h
	ld	l, -3920(ix)
	ld	h, -3919(ix)
	dec	sp
	dec	sp
	ld	-3922(ix), l
	ld	-3921(ix), h
	jp	__xcc_L864
__xcc_L863:
	ld	hl, #1
	ld	-3922(ix), l
	ld	-3921(ix), h
__xcc_L864:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3924(ix), l
	ld	-3923(ix), h
	.globl __mul16
	ld	l, -3924(ix)
	ld	h, -3923(ix)
	push	hl
	ld	l, -3922(ix)
	ld	h, -3921(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3926(ix), l
	ld	-3925(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3934(ix), l
	ld	-3933(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3936(ix), l
	ld	-3935(ix), h
	ld	l, -3934(ix)
	ld	h, -3933(ix)
	push	hl
	ld	l, -3936(ix)
	ld	h, -3935(ix)
	ld	b, l
	pop	hl
__shift_5981:
	ld	a, b
	or	a, a
	jp	z, __sdone_2475
	add	hl, hl
	djnz	__shift_5981
__sdone_2475:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3944(ix), l
	ld	-3943(ix), h
	ld	l, -3944(ix)
	ld	h, -3943(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_53666
	ld	hl, #0
	jp	__cmp_e_68011
__cmp_t_53666:
	ld	hl, #1
__cmp_e_68011:
	dec	sp
	dec	sp
	ld	-3946(ix), l
	ld	-3945(ix), h
	ld	l, -3946(ix)
	ld	h, -3945(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88967
	ld	hl, #0
	jp	__cmp_e_46912
__cmp_t_88967:
	ld	hl, #1
__cmp_e_46912:
	dec	sp
	dec	sp
	ld	-3948(ix), l
	ld	-3947(ix), h
	ld	l, -3948(ix)
	ld	h, -3947(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L871
	jp	__xcc_L872
__xcc_L872:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3956(ix), l
	ld	-3955(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-3958(ix), l
	ld	-3957(ix), h
	ld	l, -3956(ix)
	ld	h, -3955(ix)
	push	hl
	ld	l, -3958(ix)
	ld	h, -3957(ix)
	ld	b, l
	pop	hl
__shift_192:
	ld	a, b
	or	a, a
	jp	z, __sdone_1729
	add	hl, hl
	djnz	__shift_192
__sdone_1729:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3966(ix), l
	ld	-3965(ix), h
	ld	l, -3966(ix)
	ld	h, -3965(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3968(ix), l
	ld	-3967(ix), h
	ld	l, -3968(ix)
	ld	h, -3967(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80902
	ld	hl, #0
	jp	__cmp_e_56868
__cmp_t_80902:
	ld	hl, #1
__cmp_e_56868:
	dec	sp
	dec	sp
	ld	-3970(ix), l
	ld	-3969(ix), h
	ld	l, -3970(ix)
	ld	h, -3969(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88131
	ld	hl, #0
	jp	__cmp_e_65002
__cmp_t_88131:
	ld	hl, #1
__cmp_e_65002:
	dec	sp
	dec	sp
	ld	-3972(ix), l
	ld	-3971(ix), h
	jp	__xcc_L873
__xcc_L871:
	ld	hl, #1
	ld	-3972(ix), l
	ld	-3971(ix), h
__xcc_L873:
	ld	l, -3972(ix)
	ld	h, -3971(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L868
	jp	__xcc_L869
__xcc_L868:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-3974(ix), l
	ld	-3973(ix), h
	ld	l, -3974(ix)
	ld	h, -3973(ix)
	dec	sp
	dec	sp
	ld	-3976(ix), l
	ld	-3975(ix), h
	jp	__xcc_L870
__xcc_L869:
	ld	hl, #1
	ld	-3976(ix), l
	ld	-3975(ix), h
__xcc_L870:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-3978(ix), l
	ld	-3977(ix), h
	.globl __mul16
	ld	l, -3978(ix)
	ld	h, -3977(ix)
	push	hl
	ld	l, -3976(ix)
	ld	h, -3975(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-3980(ix), l
	ld	-3979(ix), h
	ld	l, -3980(ix)
	ld	h, -3979(ix)
	push	hl
	ld	l, -3926(ix)
	ld	h, -3925(ix)
	push	hl
	ld	l, -3892(ix)
	ld	h, -3891(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_874
	dec	sp
	dec	sp
	ld	-3982(ix), l
	ld	-3981(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-3990(ix), l
	ld	-3989(ix), h
	ld	l, -3990(ix)
	ld	h, -3989(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_83174
	ld	hl, #0
	jp	__cmp_e_49207
__cmp_t_83174:
	ld	hl, #1
__cmp_e_49207:
	dec	sp
	dec	sp
	ld	-3992(ix), l
	ld	-3991(ix), h
	ld	l, -3992(ix)
	ld	h, -3991(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_49718
	ld	hl, #0
	jp	__cmp_e_5216
__cmp_t_49718:
	ld	hl, #1
__cmp_e_5216:
	dec	sp
	dec	sp
	ld	-3994(ix), l
	ld	-3993(ix), h
	ld	l, -3994(ix)
	ld	h, -3993(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L878
	jp	__xcc_L879
__xcc_L879:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4002(ix), l
	ld	-4001(ix), h
	ld	l, -4002(ix)
	ld	h, -4001(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4004(ix), l
	ld	-4003(ix), h
	ld	l, -4004(ix)
	ld	h, -4003(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1183
	ld	hl, #0
	jp	__cmp_e_92377
__cmp_t_1183:
	ld	hl, #1
__cmp_e_92377:
	dec	sp
	dec	sp
	ld	-4006(ix), l
	ld	-4005(ix), h
	ld	l, -4006(ix)
	ld	h, -4005(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_45487
	ld	hl, #0
	jp	__cmp_e_57472
__cmp_t_45487:
	ld	hl, #1
__cmp_e_57472:
	dec	sp
	dec	sp
	ld	-4008(ix), l
	ld	-4007(ix), h
	jp	__xcc_L880
__xcc_L878:
	ld	hl, #1
	ld	-4008(ix), l
	ld	-4007(ix), h
__xcc_L880:
	ld	l, -4008(ix)
	ld	h, -4007(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L875
	jp	__xcc_L876
__xcc_L875:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4010(ix), l
	ld	-4009(ix), h
	ld	l, -4010(ix)
	ld	h, -4009(ix)
	dec	sp
	dec	sp
	ld	-4012(ix), l
	ld	-4011(ix), h
	jp	__xcc_L877
__xcc_L876:
	ld	hl, #1
	ld	-4012(ix), l
	ld	-4011(ix), h
__xcc_L877:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4014(ix), l
	ld	-4013(ix), h
	.globl __mul16
	ld	l, -4014(ix)
	ld	h, -4013(ix)
	push	hl
	ld	l, -4012(ix)
	ld	h, -4011(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4016(ix), l
	ld	-4015(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4024(ix), l
	ld	-4023(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4026(ix), l
	ld	-4025(ix), h
	ld	l, -4024(ix)
	ld	h, -4023(ix)
	push	hl
	ld	l, -4026(ix)
	ld	h, -4025(ix)
	ld	b, l
	pop	hl
__shift_4573:
	ld	a, b
	or	a, a
	jp	z, __sdone_8957
	add	hl, hl
	djnz	__shift_4573
__sdone_8957:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4034(ix), l
	ld	-4033(ix), h
	ld	l, -4034(ix)
	ld	h, -4033(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_25062
	ld	hl, #0
	jp	__cmp_e_76125
__cmp_t_25062:
	ld	hl, #1
__cmp_e_76125:
	dec	sp
	dec	sp
	ld	-4036(ix), l
	ld	-4035(ix), h
	ld	l, -4036(ix)
	ld	h, -4035(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13933
	ld	hl, #0
	jp	__cmp_e_66797
__cmp_t_13933:
	ld	hl, #1
__cmp_e_66797:
	dec	sp
	dec	sp
	ld	-4038(ix), l
	ld	-4037(ix), h
	ld	l, -4038(ix)
	ld	h, -4037(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L884
	jp	__xcc_L885
__xcc_L885:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4046(ix), l
	ld	-4045(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4048(ix), l
	ld	-4047(ix), h
	ld	l, -4046(ix)
	ld	h, -4045(ix)
	push	hl
	ld	l, -4048(ix)
	ld	h, -4047(ix)
	ld	b, l
	pop	hl
__shift_2496:
	ld	a, b
	or	a, a
	jp	z, __sdone_3418
	add	hl, hl
	djnz	__shift_2496
__sdone_3418:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4056(ix), l
	ld	-4055(ix), h
	ld	l, -4056(ix)
	ld	h, -4055(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4058(ix), l
	ld	-4057(ix), h
	ld	l, -4058(ix)
	ld	h, -4057(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_93141
	ld	hl, #0
	jp	__cmp_e_48153
__cmp_t_93141:
	ld	hl, #1
__cmp_e_48153:
	dec	sp
	dec	sp
	ld	-4060(ix), l
	ld	-4059(ix), h
	ld	l, -4060(ix)
	ld	h, -4059(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71726
	ld	hl, #0
	jp	__cmp_e_75474
__cmp_t_71726:
	ld	hl, #1
__cmp_e_75474:
	dec	sp
	dec	sp
	ld	-4062(ix), l
	ld	-4061(ix), h
	jp	__xcc_L886
__xcc_L884:
	ld	hl, #1
	ld	-4062(ix), l
	ld	-4061(ix), h
__xcc_L886:
	ld	l, -4062(ix)
	ld	h, -4061(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L881
	jp	__xcc_L882
__xcc_L881:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4064(ix), l
	ld	-4063(ix), h
	ld	l, -4064(ix)
	ld	h, -4063(ix)
	dec	sp
	dec	sp
	ld	-4066(ix), l
	ld	-4065(ix), h
	jp	__xcc_L883
__xcc_L882:
	ld	hl, #1
	ld	-4066(ix), l
	ld	-4065(ix), h
__xcc_L883:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4068(ix), l
	ld	-4067(ix), h
	.globl __mul16
	ld	l, -4068(ix)
	ld	h, -4067(ix)
	push	hl
	ld	l, -4066(ix)
	ld	h, -4065(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4070(ix), l
	ld	-4069(ix), h
	ld	l, -4070(ix)
	ld	h, -4069(ix)
	push	hl
	ld	l, -4016(ix)
	ld	h, -4015(ix)
	push	hl
	ld	l, -3982(ix)
	ld	h, -3981(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L859:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L858
	jp	__xcc_L860
__xcc_L860:
__xcc_L887:
	ld	hl, #__str_890
	dec	sp
	dec	sp
	ld	-4072(ix), l
	ld	-4071(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4080(ix), l
	ld	-4079(ix), h
	ld	l, -4080(ix)
	ld	h, -4079(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_96980
	ld	hl, #0
	jp	__cmp_e_25393
__cmp_t_96980:
	ld	hl, #1
__cmp_e_25393:
	dec	sp
	dec	sp
	ld	-4082(ix), l
	ld	-4081(ix), h
	ld	l, -4082(ix)
	ld	h, -4081(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43485
	ld	hl, #0
	jp	__cmp_e_85948
__cmp_t_43485:
	ld	hl, #1
__cmp_e_85948:
	dec	sp
	dec	sp
	ld	-4084(ix), l
	ld	-4083(ix), h
	ld	l, -4084(ix)
	ld	h, -4083(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L894
	jp	__xcc_L895
__xcc_L895:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4092(ix), l
	ld	-4091(ix), h
	ld	l, -4092(ix)
	ld	h, -4091(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4094(ix), l
	ld	-4093(ix), h
	ld	l, -4094(ix)
	ld	h, -4093(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72305
	ld	hl, #0
	jp	__cmp_e_50030
__cmp_t_72305:
	ld	hl, #1
__cmp_e_50030:
	dec	sp
	dec	sp
	ld	-4096(ix), l
	ld	-4095(ix), h
	ld	l, -4096(ix)
	ld	h, -4095(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_64029
	ld	hl, #0
	jp	__cmp_e_69559
__cmp_t_64029:
	ld	hl, #1
__cmp_e_69559:
	dec	sp
	dec	sp
	ld	-4098(ix), l
	ld	-4097(ix), h
	jp	__xcc_L896
__xcc_L894:
	ld	hl, #1
	ld	-4098(ix), l
	ld	-4097(ix), h
__xcc_L896:
	ld	l, -4098(ix)
	ld	h, -4097(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L891
	jp	__xcc_L892
__xcc_L891:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4100(ix), l
	ld	-4099(ix), h
	ld	l, -4100(ix)
	ld	h, -4099(ix)
	dec	sp
	dec	sp
	ld	-4102(ix), l
	ld	-4101(ix), h
	jp	__xcc_L893
__xcc_L892:
	ld	hl, #1
	ld	-4102(ix), l
	ld	-4101(ix), h
__xcc_L893:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4104(ix), l
	ld	-4103(ix), h
	.globl __mul16
	ld	l, -4104(ix)
	ld	h, -4103(ix)
	push	hl
	ld	l, -4102(ix)
	ld	h, -4101(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4106(ix), l
	ld	-4105(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4114(ix), l
	ld	-4113(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4116(ix), l
	ld	-4115(ix), h
	ld	l, -4114(ix)
	ld	h, -4113(ix)
	push	hl
	ld	l, -4116(ix)
	ld	h, -4115(ix)
	ld	b, l
	pop	hl
__shift_6898:
	ld	a, b
	or	a, a
	jp	z, __sdone_2160
	add	hl, hl
	djnz	__shift_6898
__sdone_2160:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4124(ix), l
	ld	-4123(ix), h
	ld	l, -4124(ix)
	ld	h, -4123(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_34562
	ld	hl, #0
	jp	__cmp_e_6424
__cmp_t_34562:
	ld	hl, #1
__cmp_e_6424:
	dec	sp
	dec	sp
	ld	-4126(ix), l
	ld	-4125(ix), h
	ld	l, -4126(ix)
	ld	h, -4125(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17719
	ld	hl, #0
	jp	__cmp_e_84280
__cmp_t_17719:
	ld	hl, #1
__cmp_e_84280:
	dec	sp
	dec	sp
	ld	-4128(ix), l
	ld	-4127(ix), h
	ld	l, -4128(ix)
	ld	h, -4127(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L900
	jp	__xcc_L901
__xcc_L901:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4136(ix), l
	ld	-4135(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4138(ix), l
	ld	-4137(ix), h
	ld	l, -4136(ix)
	ld	h, -4135(ix)
	push	hl
	ld	l, -4138(ix)
	ld	h, -4137(ix)
	ld	b, l
	pop	hl
__shift_1641:
	ld	a, b
	or	a, a
	jp	z, __sdone_8902
	add	hl, hl
	djnz	__shift_1641
__sdone_8902:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4146(ix), l
	ld	-4145(ix), h
	ld	l, -4146(ix)
	ld	h, -4145(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4148(ix), l
	ld	-4147(ix), h
	ld	l, -4148(ix)
	ld	h, -4147(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_93010
	ld	hl, #0
	jp	__cmp_e_73480
__cmp_t_93010:
	ld	hl, #1
__cmp_e_73480:
	dec	sp
	dec	sp
	ld	-4150(ix), l
	ld	-4149(ix), h
	ld	l, -4150(ix)
	ld	h, -4149(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92726
	ld	hl, #0
	jp	__cmp_e_27583
__cmp_t_92726:
	ld	hl, #1
__cmp_e_27583:
	dec	sp
	dec	sp
	ld	-4152(ix), l
	ld	-4151(ix), h
	jp	__xcc_L902
__xcc_L900:
	ld	hl, #1
	ld	-4152(ix), l
	ld	-4151(ix), h
__xcc_L902:
	ld	l, -4152(ix)
	ld	h, -4151(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L897
	jp	__xcc_L898
__xcc_L897:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4154(ix), l
	ld	-4153(ix), h
	ld	l, -4154(ix)
	ld	h, -4153(ix)
	dec	sp
	dec	sp
	ld	-4156(ix), l
	ld	-4155(ix), h
	jp	__xcc_L899
__xcc_L898:
	ld	hl, #1
	ld	-4156(ix), l
	ld	-4155(ix), h
__xcc_L899:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4158(ix), l
	ld	-4157(ix), h
	.globl __mul16
	ld	l, -4158(ix)
	ld	h, -4157(ix)
	push	hl
	ld	l, -4156(ix)
	ld	h, -4155(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4160(ix), l
	ld	-4159(ix), h
	ld	l, -4160(ix)
	ld	h, -4159(ix)
	push	hl
	ld	l, -4106(ix)
	ld	h, -4105(ix)
	push	hl
	ld	l, -4072(ix)
	ld	h, -4071(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_903
	dec	sp
	dec	sp
	ld	-4162(ix), l
	ld	-4161(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4170(ix), l
	ld	-4169(ix), h
	ld	l, -4170(ix)
	ld	h, -4169(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28789
	ld	hl, #0
	jp	__cmp_e_34140
__cmp_t_28789:
	ld	hl, #1
__cmp_e_34140:
	dec	sp
	dec	sp
	ld	-4172(ix), l
	ld	-4171(ix), h
	ld	l, -4172(ix)
	ld	h, -4171(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3708
	ld	hl, #0
	jp	__cmp_e_42723
__cmp_t_3708:
	ld	hl, #1
__cmp_e_42723:
	dec	sp
	dec	sp
	ld	-4174(ix), l
	ld	-4173(ix), h
	ld	l, -4174(ix)
	ld	h, -4173(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L907
	jp	__xcc_L908
__xcc_L908:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4182(ix), l
	ld	-4181(ix), h
	ld	l, -4182(ix)
	ld	h, -4181(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4184(ix), l
	ld	-4183(ix), h
	ld	l, -4184(ix)
	ld	h, -4183(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_938
	ld	hl, #0
	jp	__cmp_e_32557
__cmp_t_938:
	ld	hl, #1
__cmp_e_32557:
	dec	sp
	dec	sp
	ld	-4186(ix), l
	ld	-4185(ix), h
	ld	l, -4186(ix)
	ld	h, -4185(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52493
	ld	hl, #0
	jp	__cmp_e_10431
__cmp_t_52493:
	ld	hl, #1
__cmp_e_10431:
	dec	sp
	dec	sp
	ld	-4188(ix), l
	ld	-4187(ix), h
	jp	__xcc_L909
__xcc_L907:
	ld	hl, #1
	ld	-4188(ix), l
	ld	-4187(ix), h
__xcc_L909:
	ld	l, -4188(ix)
	ld	h, -4187(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L904
	jp	__xcc_L905
__xcc_L904:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4190(ix), l
	ld	-4189(ix), h
	ld	l, -4190(ix)
	ld	h, -4189(ix)
	dec	sp
	dec	sp
	ld	-4192(ix), l
	ld	-4191(ix), h
	jp	__xcc_L906
__xcc_L905:
	ld	hl, #1
	ld	-4192(ix), l
	ld	-4191(ix), h
__xcc_L906:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4194(ix), l
	ld	-4193(ix), h
	.globl __mul16
	ld	l, -4194(ix)
	ld	h, -4193(ix)
	push	hl
	ld	l, -4192(ix)
	ld	h, -4191(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4196(ix), l
	ld	-4195(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4204(ix), l
	ld	-4203(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4206(ix), l
	ld	-4205(ix), h
	ld	l, -4204(ix)
	ld	h, -4203(ix)
	push	hl
	ld	l, -4206(ix)
	ld	h, -4205(ix)
	ld	b, l
	pop	hl
__shift_710:
	ld	a, b
	or	a, a
	jp	z, __sdone_4220
	add	hl, hl
	djnz	__shift_710
__sdone_4220:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4214(ix), l
	ld	-4213(ix), h
	ld	l, -4214(ix)
	ld	h, -4213(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85905
	ld	hl, #0
	jp	__cmp_e_77690
__cmp_t_85905:
	ld	hl, #1
__cmp_e_77690:
	dec	sp
	dec	sp
	ld	-4216(ix), l
	ld	-4215(ix), h
	ld	l, -4216(ix)
	ld	h, -4215(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_49613
	ld	hl, #0
	jp	__cmp_e_29391
__cmp_t_49613:
	ld	hl, #1
__cmp_e_29391:
	dec	sp
	dec	sp
	ld	-4218(ix), l
	ld	-4217(ix), h
	ld	l, -4218(ix)
	ld	h, -4217(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L913
	jp	__xcc_L914
__xcc_L914:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4226(ix), l
	ld	-4225(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4228(ix), l
	ld	-4227(ix), h
	ld	l, -4226(ix)
	ld	h, -4225(ix)
	push	hl
	ld	l, -4228(ix)
	ld	h, -4227(ix)
	ld	b, l
	pop	hl
__shift_3638:
	ld	a, b
	or	a, a
	jp	z, __sdone_8270
	add	hl, hl
	djnz	__shift_3638
__sdone_8270:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4236(ix), l
	ld	-4235(ix), h
	ld	l, -4236(ix)
	ld	h, -4235(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4238(ix), l
	ld	-4237(ix), h
	ld	l, -4238(ix)
	ld	h, -4237(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79421
	ld	hl, #0
	jp	__cmp_e_27667
__cmp_t_79421:
	ld	hl, #1
__cmp_e_27667:
	dec	sp
	dec	sp
	ld	-4240(ix), l
	ld	-4239(ix), h
	ld	l, -4240(ix)
	ld	h, -4239(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7829
	ld	hl, #0
	jp	__cmp_e_2671
__cmp_t_7829:
	ld	hl, #1
__cmp_e_2671:
	dec	sp
	dec	sp
	ld	-4242(ix), l
	ld	-4241(ix), h
	jp	__xcc_L915
__xcc_L913:
	ld	hl, #1
	ld	-4242(ix), l
	ld	-4241(ix), h
__xcc_L915:
	ld	l, -4242(ix)
	ld	h, -4241(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L910
	jp	__xcc_L911
__xcc_L910:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4244(ix), l
	ld	-4243(ix), h
	ld	l, -4244(ix)
	ld	h, -4243(ix)
	dec	sp
	dec	sp
	ld	-4246(ix), l
	ld	-4245(ix), h
	jp	__xcc_L912
__xcc_L911:
	ld	hl, #1
	ld	-4246(ix), l
	ld	-4245(ix), h
__xcc_L912:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4248(ix), l
	ld	-4247(ix), h
	.globl __mul16
	ld	l, -4248(ix)
	ld	h, -4247(ix)
	push	hl
	ld	l, -4246(ix)
	ld	h, -4245(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4250(ix), l
	ld	-4249(ix), h
	ld	l, -4250(ix)
	ld	h, -4249(ix)
	push	hl
	ld	l, -4196(ix)
	ld	h, -4195(ix)
	push	hl
	ld	l, -4162(ix)
	ld	h, -4161(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L888:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L887
	jp	__xcc_L889
__xcc_L889:
__xcc_L916:
	ld	hl, #__str_919
	dec	sp
	dec	sp
	ld	-4252(ix), l
	ld	-4251(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4260(ix), l
	ld	-4259(ix), h
	ld	l, -4260(ix)
	ld	h, -4259(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_96180
	ld	hl, #0
	jp	__cmp_e_58743
__cmp_t_96180:
	ld	hl, #1
__cmp_e_58743:
	dec	sp
	dec	sp
	ld	-4262(ix), l
	ld	-4261(ix), h
	ld	l, -4262(ix)
	ld	h, -4261(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_9095
	ld	hl, #0
	jp	__cmp_e_13899
__cmp_t_9095:
	ld	hl, #1
__cmp_e_13899:
	dec	sp
	dec	sp
	ld	-4264(ix), l
	ld	-4263(ix), h
	ld	l, -4264(ix)
	ld	h, -4263(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L923
	jp	__xcc_L924
__xcc_L924:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4272(ix), l
	ld	-4271(ix), h
	ld	l, -4272(ix)
	ld	h, -4271(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4274(ix), l
	ld	-4273(ix), h
	ld	l, -4274(ix)
	ld	h, -4273(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_43024
	ld	hl, #0
	jp	__cmp_e_37088
__cmp_t_43024:
	ld	hl, #1
__cmp_e_37088:
	dec	sp
	dec	sp
	ld	-4276(ix), l
	ld	-4275(ix), h
	ld	l, -4276(ix)
	ld	h, -4275(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_49154
	ld	hl, #0
	jp	__cmp_e_52386
__cmp_t_49154:
	ld	hl, #1
__cmp_e_52386:
	dec	sp
	dec	sp
	ld	-4278(ix), l
	ld	-4277(ix), h
	jp	__xcc_L925
__xcc_L923:
	ld	hl, #1
	ld	-4278(ix), l
	ld	-4277(ix), h
__xcc_L925:
	ld	l, -4278(ix)
	ld	h, -4277(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L920
	jp	__xcc_L921
__xcc_L920:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4280(ix), l
	ld	-4279(ix), h
	ld	l, -4280(ix)
	ld	h, -4279(ix)
	dec	sp
	dec	sp
	ld	-4282(ix), l
	ld	-4281(ix), h
	jp	__xcc_L922
__xcc_L921:
	ld	hl, #1
	ld	-4282(ix), l
	ld	-4281(ix), h
__xcc_L922:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4284(ix), l
	ld	-4283(ix), h
	.globl __mul16
	ld	l, -4284(ix)
	ld	h, -4283(ix)
	push	hl
	ld	l, -4282(ix)
	ld	h, -4281(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4286(ix), l
	ld	-4285(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4294(ix), l
	ld	-4293(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4298(ix), l
	ld	-4297(ix), h
	ld	l, -4294(ix)
	ld	h, -4293(ix)
	push	hl
	ld	l, -4298(ix)
	ld	h, -4297(ix)
	ld	b, l
	pop	hl
__shift_569:
	ld	a, b
	or	a, a
	jp	z, __sdone_8232
	add	hl, hl
	djnz	__shift_569
__sdone_8232:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4306(ix), l
	ld	-4305(ix), h
	ld	l, -4306(ix)
	ld	h, -4305(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79969
	ld	hl, #0
	jp	__cmp_e_55710
__cmp_t_79969:
	ld	hl, #1
__cmp_e_55710:
	dec	sp
	dec	sp
	ld	-4308(ix), l
	ld	-4307(ix), h
	ld	l, -4308(ix)
	ld	h, -4307(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92373
	ld	hl, #0
	jp	__cmp_e_30
__cmp_t_92373:
	ld	hl, #1
__cmp_e_30:
	dec	sp
	dec	sp
	ld	-4310(ix), l
	ld	-4309(ix), h
	ld	l, -4310(ix)
	ld	h, -4309(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L929
	jp	__xcc_L930
__xcc_L930:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4318(ix), l
	ld	-4317(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4322(ix), l
	ld	-4321(ix), h
	ld	l, -4318(ix)
	ld	h, -4317(ix)
	push	hl
	ld	l, -4322(ix)
	ld	h, -4321(ix)
	ld	b, l
	pop	hl
__shift_8433:
	ld	a, b
	or	a, a
	jp	z, __sdone_9663
	add	hl, hl
	djnz	__shift_8433
__sdone_9663:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4330(ix), l
	ld	-4329(ix), h
	ld	l, -4330(ix)
	ld	h, -4329(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4332(ix), l
	ld	-4331(ix), h
	ld	l, -4332(ix)
	ld	h, -4331(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_32587
	ld	hl, #0
	jp	__cmp_e_67279
__cmp_t_32587:
	ld	hl, #1
__cmp_e_67279:
	dec	sp
	dec	sp
	ld	-4334(ix), l
	ld	-4333(ix), h
	ld	l, -4334(ix)
	ld	h, -4333(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_20094
	ld	hl, #0
	jp	__cmp_e_29649
__cmp_t_20094:
	ld	hl, #1
__cmp_e_29649:
	dec	sp
	dec	sp
	ld	-4336(ix), l
	ld	-4335(ix), h
	jp	__xcc_L931
__xcc_L929:
	ld	hl, #1
	ld	-4336(ix), l
	ld	-4335(ix), h
__xcc_L931:
	ld	l, -4336(ix)
	ld	h, -4335(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L926
	jp	__xcc_L927
__xcc_L926:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4338(ix), l
	ld	-4337(ix), h
	ld	l, -4338(ix)
	ld	h, -4337(ix)
	dec	sp
	dec	sp
	ld	-4340(ix), l
	ld	-4339(ix), h
	jp	__xcc_L928
__xcc_L927:
	ld	hl, #1
	ld	-4340(ix), l
	ld	-4339(ix), h
__xcc_L928:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4342(ix), l
	ld	-4341(ix), h
	.globl __mul16
	ld	l, -4342(ix)
	ld	h, -4341(ix)
	push	hl
	ld	l, -4340(ix)
	ld	h, -4339(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4344(ix), l
	ld	-4343(ix), h
	ld	l, -4344(ix)
	ld	h, -4343(ix)
	push	hl
	ld	l, -4286(ix)
	ld	h, -4285(ix)
	push	hl
	ld	l, -4252(ix)
	ld	h, -4251(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_932
	dec	sp
	dec	sp
	ld	-4346(ix), l
	ld	-4345(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4354(ix), l
	ld	-4353(ix), h
	ld	l, -4354(ix)
	ld	h, -4353(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91499
	ld	hl, #0
	jp	__cmp_e_22351
__cmp_t_91499:
	ld	hl, #1
__cmp_e_22351:
	dec	sp
	dec	sp
	ld	-4356(ix), l
	ld	-4355(ix), h
	ld	l, -4356(ix)
	ld	h, -4355(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7339
	ld	hl, #0
	jp	__cmp_e_57464
__cmp_t_7339:
	ld	hl, #1
__cmp_e_57464:
	dec	sp
	dec	sp
	ld	-4358(ix), l
	ld	-4357(ix), h
	ld	l, -4358(ix)
	ld	h, -4357(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L936
	jp	__xcc_L937
__xcc_L937:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4366(ix), l
	ld	-4365(ix), h
	ld	l, -4366(ix)
	ld	h, -4365(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4368(ix), l
	ld	-4367(ix), h
	ld	l, -4368(ix)
	ld	h, -4367(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51742
	ld	hl, #0
	jp	__cmp_e_87330
__cmp_t_51742:
	ld	hl, #1
__cmp_e_87330:
	dec	sp
	dec	sp
	ld	-4370(ix), l
	ld	-4369(ix), h
	ld	l, -4370(ix)
	ld	h, -4369(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_12086
	ld	hl, #0
	jp	__cmp_e_47515
__cmp_t_12086:
	ld	hl, #1
__cmp_e_47515:
	dec	sp
	dec	sp
	ld	-4372(ix), l
	ld	-4371(ix), h
	jp	__xcc_L938
__xcc_L936:
	ld	hl, #1
	ld	-4372(ix), l
	ld	-4371(ix), h
__xcc_L938:
	ld	l, -4372(ix)
	ld	h, -4371(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L933
	jp	__xcc_L934
__xcc_L933:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4374(ix), l
	ld	-4373(ix), h
	ld	l, -4374(ix)
	ld	h, -4373(ix)
	dec	sp
	dec	sp
	ld	-4376(ix), l
	ld	-4375(ix), h
	jp	__xcc_L935
__xcc_L934:
	ld	hl, #1
	ld	-4376(ix), l
	ld	-4375(ix), h
__xcc_L935:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4378(ix), l
	ld	-4377(ix), h
	.globl __mul16
	ld	l, -4378(ix)
	ld	h, -4377(ix)
	push	hl
	ld	l, -4376(ix)
	ld	h, -4375(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4380(ix), l
	ld	-4379(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4388(ix), l
	ld	-4387(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4392(ix), l
	ld	-4391(ix), h
	ld	l, -4388(ix)
	ld	h, -4387(ix)
	push	hl
	ld	l, -4392(ix)
	ld	h, -4391(ix)
	ld	b, l
	pop	hl
__shift_1349:
	ld	a, b
	or	a, a
	jp	z, __sdone_9915
	add	hl, hl
	djnz	__shift_1349
__sdone_9915:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4400(ix), l
	ld	-4399(ix), h
	ld	l, -4400(ix)
	ld	h, -4399(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_50186
	ld	hl, #0
	jp	__cmp_e_43881
__cmp_t_50186:
	ld	hl, #1
__cmp_e_43881:
	dec	sp
	dec	sp
	ld	-4402(ix), l
	ld	-4401(ix), h
	ld	l, -4402(ix)
	ld	h, -4401(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95011
	ld	hl, #0
	jp	__cmp_e_75634
__cmp_t_95011:
	ld	hl, #1
__cmp_e_75634:
	dec	sp
	dec	sp
	ld	-4404(ix), l
	ld	-4403(ix), h
	ld	l, -4404(ix)
	ld	h, -4403(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L942
	jp	__xcc_L943
__xcc_L943:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4412(ix), l
	ld	-4411(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4416(ix), l
	ld	-4415(ix), h
	ld	l, -4412(ix)
	ld	h, -4411(ix)
	push	hl
	ld	l, -4416(ix)
	ld	h, -4415(ix)
	ld	b, l
	pop	hl
__shift_4133:
	ld	a, b
	or	a, a
	jp	z, __sdone_4387
	add	hl, hl
	djnz	__shift_4133
__sdone_4387:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4424(ix), l
	ld	-4423(ix), h
	ld	l, -4424(ix)
	ld	h, -4423(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4426(ix), l
	ld	-4425(ix), h
	ld	l, -4426(ix)
	ld	h, -4425(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_12722
	ld	hl, #0
	jp	__cmp_e_23287
__cmp_t_12722:
	ld	hl, #1
__cmp_e_23287:
	dec	sp
	dec	sp
	ld	-4428(ix), l
	ld	-4427(ix), h
	ld	l, -4428(ix)
	ld	h, -4427(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6773
	ld	hl, #0
	jp	__cmp_e_39643
__cmp_t_6773:
	ld	hl, #1
__cmp_e_39643:
	dec	sp
	dec	sp
	ld	-4430(ix), l
	ld	-4429(ix), h
	jp	__xcc_L944
__xcc_L942:
	ld	hl, #1
	ld	-4430(ix), l
	ld	-4429(ix), h
__xcc_L944:
	ld	l, -4430(ix)
	ld	h, -4429(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L939
	jp	__xcc_L940
__xcc_L939:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4432(ix), l
	ld	-4431(ix), h
	ld	l, -4432(ix)
	ld	h, -4431(ix)
	dec	sp
	dec	sp
	ld	-4434(ix), l
	ld	-4433(ix), h
	jp	__xcc_L941
__xcc_L940:
	ld	hl, #1
	ld	-4434(ix), l
	ld	-4433(ix), h
__xcc_L941:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4436(ix), l
	ld	-4435(ix), h
	.globl __mul16
	ld	l, -4436(ix)
	ld	h, -4435(ix)
	push	hl
	ld	l, -4434(ix)
	ld	h, -4433(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4438(ix), l
	ld	-4437(ix), h
	ld	l, -4438(ix)
	ld	h, -4437(ix)
	push	hl
	ld	l, -4380(ix)
	ld	h, -4379(ix)
	push	hl
	ld	l, -4346(ix)
	ld	h, -4345(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L917:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L916
	jp	__xcc_L918
__xcc_L918:
__xcc_L945:
	ld	hl, #__str_948
	dec	sp
	dec	sp
	ld	-4440(ix), l
	ld	-4439(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4448(ix), l
	ld	-4447(ix), h
	ld	l, -4448(ix)
	ld	h, -4447(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_81519
	ld	hl, #0
	jp	__cmp_e_86742
__cmp_t_81519:
	ld	hl, #1
__cmp_e_86742:
	dec	sp
	dec	sp
	ld	-4450(ix), l
	ld	-4449(ix), h
	ld	l, -4450(ix)
	ld	h, -4449(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95354
	ld	hl, #0
	jp	__cmp_e_90244
__cmp_t_95354:
	ld	hl, #1
__cmp_e_90244:
	dec	sp
	dec	sp
	ld	-4452(ix), l
	ld	-4451(ix), h
	ld	l, -4452(ix)
	ld	h, -4451(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L952
	jp	__xcc_L953
__xcc_L953:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4460(ix), l
	ld	-4459(ix), h
	ld	l, -4460(ix)
	ld	h, -4459(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4462(ix), l
	ld	-4461(ix), h
	ld	l, -4462(ix)
	ld	h, -4461(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3124
	ld	hl, #0
	jp	__cmp_e_10139
__cmp_t_3124:
	ld	hl, #1
__cmp_e_10139:
	dec	sp
	dec	sp
	ld	-4464(ix), l
	ld	-4463(ix), h
	ld	l, -4464(ix)
	ld	h, -4463(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16259
	ld	hl, #0
	jp	__cmp_e_52063
__cmp_t_16259:
	ld	hl, #1
__cmp_e_52063:
	dec	sp
	dec	sp
	ld	-4466(ix), l
	ld	-4465(ix), h
	jp	__xcc_L954
__xcc_L952:
	ld	hl, #1
	ld	-4466(ix), l
	ld	-4465(ix), h
__xcc_L954:
	ld	l, -4466(ix)
	ld	h, -4465(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L949
	jp	__xcc_L950
__xcc_L949:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4468(ix), l
	ld	-4467(ix), h
	ld	l, -4468(ix)
	ld	h, -4467(ix)
	dec	sp
	dec	sp
	ld	-4470(ix), l
	ld	-4469(ix), h
	jp	__xcc_L951
__xcc_L950:
	ld	hl, #1
	ld	-4470(ix), l
	ld	-4469(ix), h
__xcc_L951:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4472(ix), l
	ld	-4471(ix), h
	.globl __mul16
	ld	l, -4472(ix)
	ld	h, -4471(ix)
	push	hl
	ld	l, -4470(ix)
	ld	h, -4469(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4474(ix), l
	ld	-4473(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4482(ix), l
	ld	-4481(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4490(ix), l
	ld	-4489(ix), h
	ld	l, -4482(ix)
	ld	h, -4481(ix)
	push	hl
	ld	l, -4490(ix)
	ld	h, -4489(ix)
	ld	b, l
	pop	hl
__shift_7418:
	ld	a, b
	or	a, a
	jp	z, __sdone_6353
	add	hl, hl
	djnz	__shift_7418
__sdone_6353:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4498(ix), l
	ld	-4497(ix), h
	ld	l, -4498(ix)
	ld	h, -4497(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_81712
	ld	hl, #0
	jp	__cmp_e_85269
__cmp_t_81712:
	ld	hl, #1
__cmp_e_85269:
	dec	sp
	dec	sp
	ld	-4500(ix), l
	ld	-4499(ix), h
	ld	l, -4500(ix)
	ld	h, -4499(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58705
	ld	hl, #0
	jp	__cmp_e_5404
__cmp_t_58705:
	ld	hl, #1
__cmp_e_5404:
	dec	sp
	dec	sp
	ld	-4502(ix), l
	ld	-4501(ix), h
	ld	l, -4502(ix)
	ld	h, -4501(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L958
	jp	__xcc_L959
__xcc_L959:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4510(ix), l
	ld	-4509(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4518(ix), l
	ld	-4517(ix), h
	ld	l, -4510(ix)
	ld	h, -4509(ix)
	push	hl
	ld	l, -4518(ix)
	ld	h, -4517(ix)
	ld	b, l
	pop	hl
__shift_2733:
	ld	a, b
	or	a, a
	jp	z, __sdone_6799
	add	hl, hl
	djnz	__shift_2733
__sdone_6799:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4526(ix), l
	ld	-4525(ix), h
	ld	l, -4526(ix)
	ld	h, -4525(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4528(ix), l
	ld	-4527(ix), h
	ld	l, -4528(ix)
	ld	h, -4527(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92734
	ld	hl, #0
	jp	__cmp_e_54819
__cmp_t_92734:
	ld	hl, #1
__cmp_e_54819:
	dec	sp
	dec	sp
	ld	-4530(ix), l
	ld	-4529(ix), h
	ld	l, -4530(ix)
	ld	h, -4529(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74315
	ld	hl, #0
	jp	__cmp_e_40435
__cmp_t_74315:
	ld	hl, #1
__cmp_e_40435:
	dec	sp
	dec	sp
	ld	-4532(ix), l
	ld	-4531(ix), h
	jp	__xcc_L960
__xcc_L958:
	ld	hl, #1
	ld	-4532(ix), l
	ld	-4531(ix), h
__xcc_L960:
	ld	l, -4532(ix)
	ld	h, -4531(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L955
	jp	__xcc_L956
__xcc_L955:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4534(ix), l
	ld	-4533(ix), h
	ld	l, -4534(ix)
	ld	h, -4533(ix)
	dec	sp
	dec	sp
	ld	-4536(ix), l
	ld	-4535(ix), h
	jp	__xcc_L957
__xcc_L956:
	ld	hl, #1
	ld	-4536(ix), l
	ld	-4535(ix), h
__xcc_L957:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4538(ix), l
	ld	-4537(ix), h
	.globl __mul16
	ld	l, -4538(ix)
	ld	h, -4537(ix)
	push	hl
	ld	l, -4536(ix)
	ld	h, -4535(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4540(ix), l
	ld	-4539(ix), h
	ld	l, -4540(ix)
	ld	h, -4539(ix)
	push	hl
	ld	l, -4474(ix)
	ld	h, -4473(ix)
	push	hl
	ld	l, -4440(ix)
	ld	h, -4439(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_961
	dec	sp
	dec	sp
	ld	-4542(ix), l
	ld	-4541(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4550(ix), l
	ld	-4549(ix), h
	ld	l, -4550(ix)
	ld	h, -4549(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91087
	ld	hl, #0
	jp	__cmp_e_40853
__cmp_t_91087:
	ld	hl, #1
__cmp_e_40853:
	dec	sp
	dec	sp
	ld	-4552(ix), l
	ld	-4551(ix), h
	ld	l, -4552(ix)
	ld	h, -4551(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_669
	ld	hl, #0
	jp	__cmp_e_2450
__cmp_t_669:
	ld	hl, #1
__cmp_e_2450:
	dec	sp
	dec	sp
	ld	-4554(ix), l
	ld	-4553(ix), h
	ld	l, -4554(ix)
	ld	h, -4553(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L965
	jp	__xcc_L966
__xcc_L966:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4562(ix), l
	ld	-4561(ix), h
	ld	l, -4562(ix)
	ld	h, -4561(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4564(ix), l
	ld	-4563(ix), h
	ld	l, -4564(ix)
	ld	h, -4563(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16487
	ld	hl, #0
	jp	__cmp_e_74802
__cmp_t_16487:
	ld	hl, #1
__cmp_e_74802:
	dec	sp
	dec	sp
	ld	-4566(ix), l
	ld	-4565(ix), h
	ld	l, -4566(ix)
	ld	h, -4565(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_56837
	ld	hl, #0
	jp	__cmp_e_45562
__cmp_t_56837:
	ld	hl, #1
__cmp_e_45562:
	dec	sp
	dec	sp
	ld	-4568(ix), l
	ld	-4567(ix), h
	jp	__xcc_L967
__xcc_L965:
	ld	hl, #1
	ld	-4568(ix), l
	ld	-4567(ix), h
__xcc_L967:
	ld	l, -4568(ix)
	ld	h, -4567(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L962
	jp	__xcc_L963
__xcc_L962:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4570(ix), l
	ld	-4569(ix), h
	ld	l, -4570(ix)
	ld	h, -4569(ix)
	dec	sp
	dec	sp
	ld	-4572(ix), l
	ld	-4571(ix), h
	jp	__xcc_L964
__xcc_L963:
	ld	hl, #1
	ld	-4572(ix), l
	ld	-4571(ix), h
__xcc_L964:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4574(ix), l
	ld	-4573(ix), h
	.globl __mul16
	ld	l, -4574(ix)
	ld	h, -4573(ix)
	push	hl
	ld	l, -4572(ix)
	ld	h, -4571(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4576(ix), l
	ld	-4575(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4584(ix), l
	ld	-4583(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4592(ix), l
	ld	-4591(ix), h
	ld	l, -4584(ix)
	ld	h, -4583(ix)
	push	hl
	ld	l, -4592(ix)
	ld	h, -4591(ix)
	ld	b, l
	pop	hl
__shift_8089:
	ld	a, b
	or	a, a
	jp	z, __sdone_3610
	add	hl, hl
	djnz	__shift_8089
__sdone_3610:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4600(ix), l
	ld	-4599(ix), h
	ld	l, -4600(ix)
	ld	h, -4599(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85205
	ld	hl, #0
	jp	__cmp_e_95960
__cmp_t_85205:
	ld	hl, #1
__cmp_e_95960:
	dec	sp
	dec	sp
	ld	-4602(ix), l
	ld	-4601(ix), h
	ld	l, -4602(ix)
	ld	h, -4601(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66704
	ld	hl, #0
	jp	__cmp_e_96911
__cmp_t_66704:
	ld	hl, #1
__cmp_e_96911:
	dec	sp
	dec	sp
	ld	-4604(ix), l
	ld	-4603(ix), h
	ld	l, -4604(ix)
	ld	h, -4603(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L971
	jp	__xcc_L972
__xcc_L972:
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4612(ix), l
	ld	-4611(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4620(ix), l
	ld	-4619(ix), h
	ld	l, -4612(ix)
	ld	h, -4611(ix)
	push	hl
	ld	l, -4620(ix)
	ld	h, -4619(ix)
	ld	b, l
	pop	hl
__shift_2557:
	ld	a, b
	or	a, a
	jp	z, __sdone_9829
	add	hl, hl
	djnz	__shift_2557
__sdone_9829:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4628(ix), l
	ld	-4627(ix), h
	ld	l, -4628(ix)
	ld	h, -4627(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4630(ix), l
	ld	-4629(ix), h
	ld	l, -4630(ix)
	ld	h, -4629(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_23403
	ld	hl, #0
	jp	__cmp_e_18816
__cmp_t_23403:
	ld	hl, #1
__cmp_e_18816:
	dec	sp
	dec	sp
	ld	-4632(ix), l
	ld	-4631(ix), h
	ld	l, -4632(ix)
	ld	h, -4631(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_21892
	ld	hl, #0
	jp	__cmp_e_821
__cmp_t_21892:
	ld	hl, #1
__cmp_e_821:
	dec	sp
	dec	sp
	ld	-4634(ix), l
	ld	-4633(ix), h
	jp	__xcc_L973
__xcc_L971:
	ld	hl, #1
	ld	-4634(ix), l
	ld	-4633(ix), h
__xcc_L973:
	ld	l, -4634(ix)
	ld	h, -4633(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L968
	jp	__xcc_L969
__xcc_L968:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4636(ix), l
	ld	-4635(ix), h
	ld	l, -4636(ix)
	ld	h, -4635(ix)
	dec	sp
	dec	sp
	ld	-4638(ix), l
	ld	-4637(ix), h
	jp	__xcc_L970
__xcc_L969:
	ld	hl, #1
	ld	-4638(ix), l
	ld	-4637(ix), h
__xcc_L970:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-4640(ix), l
	ld	-4639(ix), h
	.globl __mul16
	ld	l, -4640(ix)
	ld	h, -4639(ix)
	push	hl
	ld	l, -4638(ix)
	ld	h, -4637(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4642(ix), l
	ld	-4641(ix), h
	ld	l, -4642(ix)
	ld	h, -4641(ix)
	push	hl
	ld	l, -4576(ix)
	ld	h, -4575(ix)
	push	hl
	ld	l, -4542(ix)
	ld	h, -4541(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L946:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L945
	jp	__xcc_L947
__xcc_L947:
__xcc_L856:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L855
	jp	__xcc_L857
__xcc_L857:
__xcc_L734:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L733
	jp	__xcc_L735
__xcc_L735:
__xcc_L8:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L7
	jp	__xcc_L9
__xcc_L9:
__xcc_L974:
__xcc_L977:
__xcc_L980:
__xcc_L983:
	ld	hl, #__str_986
	dec	sp
	dec	sp
	ld	-4644(ix), l
	ld	-4643(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4646(ix), l
	ld	-4645(ix), h
	ld	l, -4646(ix)
	ld	h, -4645(ix)
	dec	sp
	dec	sp
	ld	-4648(ix), l
	ld	-4647(ix), h
	ld	l, -4648(ix)
	ld	h, -4647(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71522
	ld	hl, #0
	jp	__cmp_e_3605
__cmp_t_71522:
	ld	hl, #1
__cmp_e_3605:
	dec	sp
	dec	sp
	ld	-4650(ix), l
	ld	-4649(ix), h
	ld	l, -4650(ix)
	ld	h, -4649(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2443
	ld	hl, #0
	jp	__cmp_e_46579
__cmp_t_2443:
	ld	hl, #1
__cmp_e_46579:
	dec	sp
	dec	sp
	ld	-4652(ix), l
	ld	-4651(ix), h
	ld	l, -4652(ix)
	ld	h, -4651(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L990
	jp	__xcc_L991
__xcc_L991:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4654(ix), l
	ld	-4653(ix), h
	ld	l, -4654(ix)
	ld	h, -4653(ix)
	dec	sp
	dec	sp
	ld	-4656(ix), l
	ld	-4655(ix), h
	ld	l, -4656(ix)
	ld	h, -4655(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4658(ix), l
	ld	-4657(ix), h
	ld	l, -4658(ix)
	ld	h, -4657(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_25361
	ld	hl, #0
	jp	__cmp_e_61528
__cmp_t_25361:
	ld	hl, #1
__cmp_e_61528:
	dec	sp
	dec	sp
	ld	-4660(ix), l
	ld	-4659(ix), h
	ld	l, -4660(ix)
	ld	h, -4659(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73378
	ld	hl, #0
	jp	__cmp_e_34447
__cmp_t_73378:
	ld	hl, #1
__cmp_e_34447:
	dec	sp
	dec	sp
	ld	-4662(ix), l
	ld	-4661(ix), h
	jp	__xcc_L992
__xcc_L990:
	ld	hl, #1
	ld	-4662(ix), l
	ld	-4661(ix), h
__xcc_L992:
	ld	l, -4662(ix)
	ld	h, -4661(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L987
	jp	__xcc_L988
__xcc_L987:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4664(ix), l
	ld	-4663(ix), h
	ld	l, -4664(ix)
	ld	h, -4663(ix)
	dec	sp
	dec	sp
	ld	-4666(ix), l
	ld	-4665(ix), h
	jp	__xcc_L989
__xcc_L988:
	ld	hl, #1
	ld	-4666(ix), l
	ld	-4665(ix), h
__xcc_L989:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4668(ix), l
	ld	-4667(ix), h
	.globl __mul16
	ld	l, -4668(ix)
	ld	h, -4667(ix)
	push	hl
	ld	l, -4666(ix)
	ld	h, -4665(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4670(ix), l
	ld	-4669(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4672(ix), l
	ld	-4671(ix), h
	ld	l, -4672(ix)
	ld	h, -4671(ix)
	dec	sp
	dec	sp
	ld	-4674(ix), l
	ld	-4673(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4676(ix), l
	ld	-4675(ix), h
	ld	l, -4674(ix)
	ld	h, -4673(ix)
	push	hl
	ld	l, -4676(ix)
	ld	h, -4675(ix)
	ld	b, l
	pop	hl
__shift_2700:
	ld	a, b
	or	a, a
	jp	z, __sdone_4045
	add	hl, hl
	djnz	__shift_2700
__sdone_4045:
	dec	sp
	dec	sp
	ld	-4678(ix), l
	ld	-4677(ix), h
	ld	l, -4678(ix)
	ld	h, -4677(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74882
	ld	hl, #0
	jp	__cmp_e_23787
__cmp_t_74882:
	ld	hl, #1
__cmp_e_23787:
	dec	sp
	dec	sp
	ld	-4680(ix), l
	ld	-4679(ix), h
	ld	l, -4680(ix)
	ld	h, -4679(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4899
	ld	hl, #0
	jp	__cmp_e_75551
__cmp_t_4899:
	ld	hl, #1
__cmp_e_75551:
	dec	sp
	dec	sp
	ld	-4682(ix), l
	ld	-4681(ix), h
	ld	l, -4682(ix)
	ld	h, -4681(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L996
	jp	__xcc_L997
__xcc_L997:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4684(ix), l
	ld	-4683(ix), h
	ld	l, -4684(ix)
	ld	h, -4683(ix)
	dec	sp
	dec	sp
	ld	-4686(ix), l
	ld	-4685(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4688(ix), l
	ld	-4687(ix), h
	ld	l, -4686(ix)
	ld	h, -4685(ix)
	push	hl
	ld	l, -4688(ix)
	ld	h, -4687(ix)
	ld	b, l
	pop	hl
__shift_2589:
	ld	a, b
	or	a, a
	jp	z, __sdone_1386
	add	hl, hl
	djnz	__shift_2589
__sdone_1386:
	dec	sp
	dec	sp
	ld	-4690(ix), l
	ld	-4689(ix), h
	ld	l, -4690(ix)
	ld	h, -4689(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4692(ix), l
	ld	-4691(ix), h
	ld	l, -4692(ix)
	ld	h, -4691(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_50353
	ld	hl, #0
	jp	__cmp_e_99426
__cmp_t_50353:
	ld	hl, #1
__cmp_e_99426:
	dec	sp
	dec	sp
	ld	-4694(ix), l
	ld	-4693(ix), h
	ld	l, -4694(ix)
	ld	h, -4693(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66948
	ld	hl, #0
	jp	__cmp_e_64794
__cmp_t_66948:
	ld	hl, #1
__cmp_e_64794:
	dec	sp
	dec	sp
	ld	-4696(ix), l
	ld	-4695(ix), h
	jp	__xcc_L998
__xcc_L996:
	ld	hl, #1
	ld	-4696(ix), l
	ld	-4695(ix), h
__xcc_L998:
	ld	l, -4696(ix)
	ld	h, -4695(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L993
	jp	__xcc_L994
__xcc_L993:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4698(ix), l
	ld	-4697(ix), h
	ld	l, -4698(ix)
	ld	h, -4697(ix)
	dec	sp
	dec	sp
	ld	-4700(ix), l
	ld	-4699(ix), h
	jp	__xcc_L995
__xcc_L994:
	ld	hl, #1
	ld	-4700(ix), l
	ld	-4699(ix), h
__xcc_L995:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4702(ix), l
	ld	-4701(ix), h
	.globl __mul16
	ld	l, -4702(ix)
	ld	h, -4701(ix)
	push	hl
	ld	l, -4700(ix)
	ld	h, -4699(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4704(ix), l
	ld	-4703(ix), h
	ld	l, -4704(ix)
	ld	h, -4703(ix)
	push	hl
	ld	l, -4670(ix)
	ld	h, -4669(ix)
	push	hl
	ld	l, -4644(ix)
	ld	h, -4643(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_999
	dec	sp
	dec	sp
	ld	-4706(ix), l
	ld	-4705(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4708(ix), l
	ld	-4707(ix), h
	ld	l, -4708(ix)
	ld	h, -4707(ix)
	dec	sp
	dec	sp
	ld	-4710(ix), l
	ld	-4709(ix), h
	ld	l, -4710(ix)
	ld	h, -4709(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63036
	ld	hl, #0
	jp	__cmp_e_68506
__cmp_t_63036:
	ld	hl, #1
__cmp_e_68506:
	dec	sp
	dec	sp
	ld	-4712(ix), l
	ld	-4711(ix), h
	ld	l, -4712(ix)
	ld	h, -4711(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_77107
	ld	hl, #0
	jp	__cmp_e_46092
__cmp_t_77107:
	ld	hl, #1
__cmp_e_46092:
	dec	sp
	dec	sp
	ld	-4714(ix), l
	ld	-4713(ix), h
	ld	l, -4714(ix)
	ld	h, -4713(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1003
	jp	__xcc_L1004
__xcc_L1004:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4716(ix), l
	ld	-4715(ix), h
	ld	l, -4716(ix)
	ld	h, -4715(ix)
	dec	sp
	dec	sp
	ld	-4718(ix), l
	ld	-4717(ix), h
	ld	l, -4718(ix)
	ld	h, -4717(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4720(ix), l
	ld	-4719(ix), h
	ld	l, -4720(ix)
	ld	h, -4719(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65417
	ld	hl, #0
	jp	__cmp_e_79664
__cmp_t_65417:
	ld	hl, #1
__cmp_e_79664:
	dec	sp
	dec	sp
	ld	-4722(ix), l
	ld	-4721(ix), h
	ld	l, -4722(ix)
	ld	h, -4721(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15921
	ld	hl, #0
	jp	__cmp_e_88820
__cmp_t_15921:
	ld	hl, #1
__cmp_e_88820:
	dec	sp
	dec	sp
	ld	-4724(ix), l
	ld	-4723(ix), h
	jp	__xcc_L1005
__xcc_L1003:
	ld	hl, #1
	ld	-4724(ix), l
	ld	-4723(ix), h
__xcc_L1005:
	ld	l, -4724(ix)
	ld	h, -4723(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1000
	jp	__xcc_L1001
__xcc_L1000:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4726(ix), l
	ld	-4725(ix), h
	ld	l, -4726(ix)
	ld	h, -4725(ix)
	dec	sp
	dec	sp
	ld	-4728(ix), l
	ld	-4727(ix), h
	jp	__xcc_L1002
__xcc_L1001:
	ld	hl, #1
	ld	-4728(ix), l
	ld	-4727(ix), h
__xcc_L1002:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4730(ix), l
	ld	-4729(ix), h
	.globl __mul16
	ld	l, -4730(ix)
	ld	h, -4729(ix)
	push	hl
	ld	l, -4728(ix)
	ld	h, -4727(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4732(ix), l
	ld	-4731(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4734(ix), l
	ld	-4733(ix), h
	ld	l, -4734(ix)
	ld	h, -4733(ix)
	dec	sp
	dec	sp
	ld	-4736(ix), l
	ld	-4735(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4738(ix), l
	ld	-4737(ix), h
	ld	l, -4736(ix)
	ld	h, -4735(ix)
	push	hl
	ld	l, -4738(ix)
	ld	h, -4737(ix)
	ld	b, l
	pop	hl
__shift_8480:
	ld	a, b
	or	a, a
	jp	z, __sdone_4166
	add	hl, hl
	djnz	__shift_8480
__sdone_4166:
	dec	sp
	dec	sp
	ld	-4740(ix), l
	ld	-4739(ix), h
	ld	l, -4740(ix)
	ld	h, -4739(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5994
	ld	hl, #0
	jp	__cmp_e_86354
__cmp_t_5994:
	ld	hl, #1
__cmp_e_86354:
	dec	sp
	dec	sp
	ld	-4742(ix), l
	ld	-4741(ix), h
	ld	l, -4742(ix)
	ld	h, -4741(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74123
	ld	hl, #0
	jp	__cmp_e_8437
__cmp_t_74123:
	ld	hl, #1
__cmp_e_8437:
	dec	sp
	dec	sp
	ld	-4744(ix), l
	ld	-4743(ix), h
	ld	l, -4744(ix)
	ld	h, -4743(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1009
	jp	__xcc_L1010
__xcc_L1010:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4746(ix), l
	ld	-4745(ix), h
	ld	l, -4746(ix)
	ld	h, -4745(ix)
	dec	sp
	dec	sp
	ld	-4748(ix), l
	ld	-4747(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4750(ix), l
	ld	-4749(ix), h
	ld	l, -4748(ix)
	ld	h, -4747(ix)
	push	hl
	ld	l, -4750(ix)
	ld	h, -4749(ix)
	ld	b, l
	pop	hl
__shift_2933:
	ld	a, b
	or	a, a
	jp	z, __sdone_9484
	add	hl, hl
	djnz	__shift_2933
__sdone_9484:
	dec	sp
	dec	sp
	ld	-4752(ix), l
	ld	-4751(ix), h
	ld	l, -4752(ix)
	ld	h, -4751(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4754(ix), l
	ld	-4753(ix), h
	ld	l, -4754(ix)
	ld	h, -4753(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_86317
	ld	hl, #0
	jp	__cmp_e_6312
__cmp_t_86317:
	ld	hl, #1
__cmp_e_6312:
	dec	sp
	dec	sp
	ld	-4756(ix), l
	ld	-4755(ix), h
	ld	l, -4756(ix)
	ld	h, -4755(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33931
	ld	hl, #0
	jp	__cmp_e_19017
__cmp_t_33931:
	ld	hl, #1
__cmp_e_19017:
	dec	sp
	dec	sp
	ld	-4758(ix), l
	ld	-4757(ix), h
	jp	__xcc_L1011
__xcc_L1009:
	ld	hl, #1
	ld	-4758(ix), l
	ld	-4757(ix), h
__xcc_L1011:
	ld	l, -4758(ix)
	ld	h, -4757(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1006
	jp	__xcc_L1007
__xcc_L1006:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4760(ix), l
	ld	-4759(ix), h
	ld	l, -4760(ix)
	ld	h, -4759(ix)
	dec	sp
	dec	sp
	ld	-4762(ix), l
	ld	-4761(ix), h
	jp	__xcc_L1008
__xcc_L1007:
	ld	hl, #1
	ld	-4762(ix), l
	ld	-4761(ix), h
__xcc_L1008:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4764(ix), l
	ld	-4763(ix), h
	.globl __mul16
	ld	l, -4764(ix)
	ld	h, -4763(ix)
	push	hl
	ld	l, -4762(ix)
	ld	h, -4761(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4766(ix), l
	ld	-4765(ix), h
	ld	l, -4766(ix)
	ld	h, -4765(ix)
	push	hl
	ld	l, -4732(ix)
	ld	h, -4731(ix)
	push	hl
	ld	l, -4706(ix)
	ld	h, -4705(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L984:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L983
	jp	__xcc_L985
__xcc_L985:
__xcc_L1012:
	ld	hl, #__str_1015
	dec	sp
	dec	sp
	ld	-4768(ix), l
	ld	-4767(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4770(ix), l
	ld	-4769(ix), h
	ld	l, -4770(ix)
	ld	h, -4769(ix)
	dec	sp
	dec	sp
	ld	-4772(ix), l
	ld	-4771(ix), h
	ld	l, -4772(ix)
	ld	h, -4771(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_86709
	ld	hl, #0
	jp	__cmp_e_25165
__cmp_t_86709:
	ld	hl, #1
__cmp_e_25165:
	dec	sp
	dec	sp
	ld	-4774(ix), l
	ld	-4773(ix), h
	ld	l, -4774(ix)
	ld	h, -4773(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_59156
	ld	hl, #0
	jp	__cmp_e_91608
__cmp_t_59156:
	ld	hl, #1
__cmp_e_91608:
	dec	sp
	dec	sp
	ld	-4776(ix), l
	ld	-4775(ix), h
	ld	l, -4776(ix)
	ld	h, -4775(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1019
	jp	__xcc_L1020
__xcc_L1020:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4778(ix), l
	ld	-4777(ix), h
	ld	l, -4778(ix)
	ld	h, -4777(ix)
	dec	sp
	dec	sp
	ld	-4780(ix), l
	ld	-4779(ix), h
	ld	l, -4780(ix)
	ld	h, -4779(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4782(ix), l
	ld	-4781(ix), h
	ld	l, -4782(ix)
	ld	h, -4781(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_17069
	ld	hl, #0
	jp	__cmp_e_1745
__cmp_t_17069:
	ld	hl, #1
__cmp_e_1745:
	dec	sp
	dec	sp
	ld	-4784(ix), l
	ld	-4783(ix), h
	ld	l, -4784(ix)
	ld	h, -4783(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_12995
	ld	hl, #0
	jp	__cmp_e_67422
__cmp_t_12995:
	ld	hl, #1
__cmp_e_67422:
	dec	sp
	dec	sp
	ld	-4786(ix), l
	ld	-4785(ix), h
	jp	__xcc_L1021
__xcc_L1019:
	ld	hl, #1
	ld	-4786(ix), l
	ld	-4785(ix), h
__xcc_L1021:
	ld	l, -4786(ix)
	ld	h, -4785(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1016
	jp	__xcc_L1017
__xcc_L1016:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4788(ix), l
	ld	-4787(ix), h
	ld	l, -4788(ix)
	ld	h, -4787(ix)
	dec	sp
	dec	sp
	ld	-4790(ix), l
	ld	-4789(ix), h
	jp	__xcc_L1018
__xcc_L1017:
	ld	hl, #1
	ld	-4790(ix), l
	ld	-4789(ix), h
__xcc_L1018:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4792(ix), l
	ld	-4791(ix), h
	.globl __mul16
	ld	l, -4792(ix)
	ld	h, -4791(ix)
	push	hl
	ld	l, -4790(ix)
	ld	h, -4789(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4794(ix), l
	ld	-4793(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4796(ix), l
	ld	-4795(ix), h
	ld	l, -4796(ix)
	ld	h, -4795(ix)
	dec	sp
	dec	sp
	ld	-4798(ix), l
	ld	-4797(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4800(ix), l
	ld	-4799(ix), h
	ld	l, -4798(ix)
	ld	h, -4797(ix)
	push	hl
	ld	l, -4800(ix)
	ld	h, -4799(ix)
	ld	b, l
	pop	hl
__shift_1171:
	ld	a, b
	or	a, a
	jp	z, __sdone_6295
	add	hl, hl
	djnz	__shift_1171
__sdone_6295:
	dec	sp
	dec	sp
	ld	-4802(ix), l
	ld	-4801(ix), h
	ld	l, -4802(ix)
	ld	h, -4801(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_48569
	ld	hl, #0
	jp	__cmp_e_80559
__cmp_t_48569:
	ld	hl, #1
__cmp_e_80559:
	dec	sp
	dec	sp
	ld	-4804(ix), l
	ld	-4803(ix), h
	ld	l, -4804(ix)
	ld	h, -4803(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_64801
	ld	hl, #0
	jp	__cmp_e_25676
__cmp_t_64801:
	ld	hl, #1
__cmp_e_25676:
	dec	sp
	dec	sp
	ld	-4806(ix), l
	ld	-4805(ix), h
	ld	l, -4806(ix)
	ld	h, -4805(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1025
	jp	__xcc_L1026
__xcc_L1026:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4808(ix), l
	ld	-4807(ix), h
	ld	l, -4808(ix)
	ld	h, -4807(ix)
	dec	sp
	dec	sp
	ld	-4810(ix), l
	ld	-4809(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4812(ix), l
	ld	-4811(ix), h
	ld	l, -4810(ix)
	ld	h, -4809(ix)
	push	hl
	ld	l, -4812(ix)
	ld	h, -4811(ix)
	ld	b, l
	pop	hl
__shift_6652:
	ld	a, b
	or	a, a
	jp	z, __sdone_6571
	add	hl, hl
	djnz	__shift_6652
__sdone_6571:
	dec	sp
	dec	sp
	ld	-4814(ix), l
	ld	-4813(ix), h
	ld	l, -4814(ix)
	ld	h, -4813(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4816(ix), l
	ld	-4815(ix), h
	ld	l, -4816(ix)
	ld	h, -4815(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5340
	ld	hl, #0
	jp	__cmp_e_58925
__cmp_t_5340:
	ld	hl, #1
__cmp_e_58925:
	dec	sp
	dec	sp
	ld	-4818(ix), l
	ld	-4817(ix), h
	ld	l, -4818(ix)
	ld	h, -4817(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51743
	ld	hl, #0
	jp	__cmp_e_20172
__cmp_t_51743:
	ld	hl, #1
__cmp_e_20172:
	dec	sp
	dec	sp
	ld	-4820(ix), l
	ld	-4819(ix), h
	jp	__xcc_L1027
__xcc_L1025:
	ld	hl, #1
	ld	-4820(ix), l
	ld	-4819(ix), h
__xcc_L1027:
	ld	l, -4820(ix)
	ld	h, -4819(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1022
	jp	__xcc_L1023
__xcc_L1022:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4822(ix), l
	ld	-4821(ix), h
	ld	l, -4822(ix)
	ld	h, -4821(ix)
	dec	sp
	dec	sp
	ld	-4824(ix), l
	ld	-4823(ix), h
	jp	__xcc_L1024
__xcc_L1023:
	ld	hl, #1
	ld	-4824(ix), l
	ld	-4823(ix), h
__xcc_L1024:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4826(ix), l
	ld	-4825(ix), h
	.globl __mul16
	ld	l, -4826(ix)
	ld	h, -4825(ix)
	push	hl
	ld	l, -4824(ix)
	ld	h, -4823(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4828(ix), l
	ld	-4827(ix), h
	ld	l, -4828(ix)
	ld	h, -4827(ix)
	push	hl
	ld	l, -4794(ix)
	ld	h, -4793(ix)
	push	hl
	ld	l, -4768(ix)
	ld	h, -4767(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1028
	dec	sp
	dec	sp
	ld	-4830(ix), l
	ld	-4829(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4832(ix), l
	ld	-4831(ix), h
	ld	l, -4832(ix)
	ld	h, -4831(ix)
	dec	sp
	dec	sp
	ld	-4834(ix), l
	ld	-4833(ix), h
	ld	l, -4834(ix)
	ld	h, -4833(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13091
	ld	hl, #0
	jp	__cmp_e_74089
__cmp_t_13091:
	ld	hl, #1
__cmp_e_74089:
	dec	sp
	dec	sp
	ld	-4836(ix), l
	ld	-4835(ix), h
	ld	l, -4836(ix)
	ld	h, -4835(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6527
	ld	hl, #0
	jp	__cmp_e_3566
__cmp_t_6527:
	ld	hl, #1
__cmp_e_3566:
	dec	sp
	dec	sp
	ld	-4838(ix), l
	ld	-4837(ix), h
	ld	l, -4838(ix)
	ld	h, -4837(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1032
	jp	__xcc_L1033
__xcc_L1033:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4840(ix), l
	ld	-4839(ix), h
	ld	l, -4840(ix)
	ld	h, -4839(ix)
	dec	sp
	dec	sp
	ld	-4842(ix), l
	ld	-4841(ix), h
	ld	l, -4842(ix)
	ld	h, -4841(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4844(ix), l
	ld	-4843(ix), h
	ld	l, -4844(ix)
	ld	h, -4843(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98878
	ld	hl, #0
	jp	__cmp_e_55812
__cmp_t_98878:
	ld	hl, #1
__cmp_e_55812:
	dec	sp
	dec	sp
	ld	-4846(ix), l
	ld	-4845(ix), h
	ld	l, -4846(ix)
	ld	h, -4845(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3050
	ld	hl, #0
	jp	__cmp_e_85196
__cmp_t_3050:
	ld	hl, #1
__cmp_e_85196:
	dec	sp
	dec	sp
	ld	-4848(ix), l
	ld	-4847(ix), h
	jp	__xcc_L1034
__xcc_L1032:
	ld	hl, #1
	ld	-4848(ix), l
	ld	-4847(ix), h
__xcc_L1034:
	ld	l, -4848(ix)
	ld	h, -4847(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1029
	jp	__xcc_L1030
__xcc_L1029:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4850(ix), l
	ld	-4849(ix), h
	ld	l, -4850(ix)
	ld	h, -4849(ix)
	dec	sp
	dec	sp
	ld	-4852(ix), l
	ld	-4851(ix), h
	jp	__xcc_L1031
__xcc_L1030:
	ld	hl, #1
	ld	-4852(ix), l
	ld	-4851(ix), h
__xcc_L1031:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4854(ix), l
	ld	-4853(ix), h
	.globl __mul16
	ld	l, -4854(ix)
	ld	h, -4853(ix)
	push	hl
	ld	l, -4852(ix)
	ld	h, -4851(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4856(ix), l
	ld	-4855(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4858(ix), l
	ld	-4857(ix), h
	ld	l, -4858(ix)
	ld	h, -4857(ix)
	dec	sp
	dec	sp
	ld	-4860(ix), l
	ld	-4859(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4862(ix), l
	ld	-4861(ix), h
	ld	l, -4860(ix)
	ld	h, -4859(ix)
	push	hl
	ld	l, -4862(ix)
	ld	h, -4861(ix)
	ld	b, l
	pop	hl
__shift_2124:
	ld	a, b
	or	a, a
	jp	z, __sdone_3333
	add	hl, hl
	djnz	__shift_2124
__sdone_3333:
	dec	sp
	dec	sp
	ld	-4864(ix), l
	ld	-4863(ix), h
	ld	l, -4864(ix)
	ld	h, -4863(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4213
	ld	hl, #0
	jp	__cmp_e_65186
__cmp_t_4213:
	ld	hl, #1
__cmp_e_65186:
	dec	sp
	dec	sp
	ld	-4866(ix), l
	ld	-4865(ix), h
	ld	l, -4866(ix)
	ld	h, -4865(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_78499
	ld	hl, #0
	jp	__cmp_e_63370
__cmp_t_78499:
	ld	hl, #1
__cmp_e_63370:
	dec	sp
	dec	sp
	ld	-4868(ix), l
	ld	-4867(ix), h
	ld	l, -4868(ix)
	ld	h, -4867(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1038
	jp	__xcc_L1039
__xcc_L1039:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4870(ix), l
	ld	-4869(ix), h
	ld	l, -4870(ix)
	ld	h, -4869(ix)
	dec	sp
	dec	sp
	ld	-4872(ix), l
	ld	-4871(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-4874(ix), l
	ld	-4873(ix), h
	ld	l, -4872(ix)
	ld	h, -4871(ix)
	push	hl
	ld	l, -4874(ix)
	ld	h, -4873(ix)
	ld	b, l
	pop	hl
__shift_6794:
	ld	a, b
	or	a, a
	jp	z, __sdone_5568
	add	hl, hl
	djnz	__shift_6794
__sdone_5568:
	dec	sp
	dec	sp
	ld	-4876(ix), l
	ld	-4875(ix), h
	ld	l, -4876(ix)
	ld	h, -4875(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4878(ix), l
	ld	-4877(ix), h
	ld	l, -4878(ix)
	ld	h, -4877(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65115
	ld	hl, #0
	jp	__cmp_e_86141
__cmp_t_65115:
	ld	hl, #1
__cmp_e_86141:
	dec	sp
	dec	sp
	ld	-4880(ix), l
	ld	-4879(ix), h
	ld	l, -4880(ix)
	ld	h, -4879(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79342
	ld	hl, #0
	jp	__cmp_e_82639
__cmp_t_79342:
	ld	hl, #1
__cmp_e_82639:
	dec	sp
	dec	sp
	ld	-4882(ix), l
	ld	-4881(ix), h
	jp	__xcc_L1040
__xcc_L1038:
	ld	hl, #1
	ld	-4882(ix), l
	ld	-4881(ix), h
__xcc_L1040:
	ld	l, -4882(ix)
	ld	h, -4881(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1035
	jp	__xcc_L1036
__xcc_L1035:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4884(ix), l
	ld	-4883(ix), h
	ld	l, -4884(ix)
	ld	h, -4883(ix)
	dec	sp
	dec	sp
	ld	-4886(ix), l
	ld	-4885(ix), h
	jp	__xcc_L1037
__xcc_L1036:
	ld	hl, #1
	ld	-4886(ix), l
	ld	-4885(ix), h
__xcc_L1037:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4888(ix), l
	ld	-4887(ix), h
	.globl __mul16
	ld	l, -4888(ix)
	ld	h, -4887(ix)
	push	hl
	ld	l, -4886(ix)
	ld	h, -4885(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4890(ix), l
	ld	-4889(ix), h
	ld	l, -4890(ix)
	ld	h, -4889(ix)
	push	hl
	ld	l, -4856(ix)
	ld	h, -4855(ix)
	push	hl
	ld	l, -4830(ix)
	ld	h, -4829(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1013:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1012
	jp	__xcc_L1014
__xcc_L1014:
__xcc_L1041:
	ld	hl, #__str_1044
	dec	sp
	dec	sp
	ld	-4892(ix), l
	ld	-4891(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4894(ix), l
	ld	-4893(ix), h
	ld	l, -4894(ix)
	ld	h, -4893(ix)
	dec	sp
	dec	sp
	ld	-4896(ix), l
	ld	-4895(ix), h
	ld	l, -4896(ix)
	ld	h, -4895(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_82437
	ld	hl, #0
	jp	__cmp_e_44263
__cmp_t_82437:
	ld	hl, #1
__cmp_e_44263:
	dec	sp
	dec	sp
	ld	-4898(ix), l
	ld	-4897(ix), h
	ld	l, -4898(ix)
	ld	h, -4897(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_63198
	ld	hl, #0
	jp	__cmp_e_63590
__cmp_t_63198:
	ld	hl, #1
__cmp_e_63590:
	dec	sp
	dec	sp
	ld	-4900(ix), l
	ld	-4899(ix), h
	ld	l, -4900(ix)
	ld	h, -4899(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1048
	jp	__xcc_L1049
__xcc_L1049:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4902(ix), l
	ld	-4901(ix), h
	ld	l, -4902(ix)
	ld	h, -4901(ix)
	dec	sp
	dec	sp
	ld	-4904(ix), l
	ld	-4903(ix), h
	ld	l, -4904(ix)
	ld	h, -4903(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4906(ix), l
	ld	-4905(ix), h
	ld	l, -4906(ix)
	ld	h, -4905(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_69939
	ld	hl, #0
	jp	__cmp_e_6202
__cmp_t_69939:
	ld	hl, #1
__cmp_e_6202:
	dec	sp
	dec	sp
	ld	-4908(ix), l
	ld	-4907(ix), h
	ld	l, -4908(ix)
	ld	h, -4907(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26513
	ld	hl, #0
	jp	__cmp_e_91631
__cmp_t_26513:
	ld	hl, #1
__cmp_e_91631:
	dec	sp
	dec	sp
	ld	-4910(ix), l
	ld	-4909(ix), h
	jp	__xcc_L1050
__xcc_L1048:
	ld	hl, #1
	ld	-4910(ix), l
	ld	-4909(ix), h
__xcc_L1050:
	ld	l, -4910(ix)
	ld	h, -4909(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1045
	jp	__xcc_L1046
__xcc_L1045:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4912(ix), l
	ld	-4911(ix), h
	ld	l, -4912(ix)
	ld	h, -4911(ix)
	dec	sp
	dec	sp
	ld	-4914(ix), l
	ld	-4913(ix), h
	jp	__xcc_L1047
__xcc_L1046:
	ld	hl, #1
	ld	-4914(ix), l
	ld	-4913(ix), h
__xcc_L1047:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4916(ix), l
	ld	-4915(ix), h
	.globl __mul16
	ld	l, -4916(ix)
	ld	h, -4915(ix)
	push	hl
	ld	l, -4914(ix)
	ld	h, -4913(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4918(ix), l
	ld	-4917(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4920(ix), l
	ld	-4919(ix), h
	ld	l, -4920(ix)
	ld	h, -4919(ix)
	dec	sp
	dec	sp
	ld	-4922(ix), l
	ld	-4921(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4926(ix), l
	ld	-4925(ix), h
	ld	l, -4922(ix)
	ld	h, -4921(ix)
	push	hl
	ld	l, -4926(ix)
	ld	h, -4925(ix)
	ld	b, l
	pop	hl
__shift_5128:
	ld	a, b
	or	a, a
	jp	z, __sdone_8257
	add	hl, hl
	djnz	__shift_5128
__sdone_8257:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4930(ix), l
	ld	-4929(ix), h
	ld	l, -4930(ix)
	ld	h, -4929(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_11804
	ld	hl, #0
	jp	__cmp_e_94571
__cmp_t_11804:
	ld	hl, #1
__cmp_e_94571:
	dec	sp
	dec	sp
	ld	-4932(ix), l
	ld	-4931(ix), h
	ld	l, -4932(ix)
	ld	h, -4931(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52346
	ld	hl, #0
	jp	__cmp_e_34683
__cmp_t_52346:
	ld	hl, #1
__cmp_e_34683:
	dec	sp
	dec	sp
	ld	-4934(ix), l
	ld	-4933(ix), h
	ld	l, -4934(ix)
	ld	h, -4933(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1054
	jp	__xcc_L1055
__xcc_L1055:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4936(ix), l
	ld	-4935(ix), h
	ld	l, -4936(ix)
	ld	h, -4935(ix)
	dec	sp
	dec	sp
	ld	-4938(ix), l
	ld	-4937(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4942(ix), l
	ld	-4941(ix), h
	ld	l, -4938(ix)
	ld	h, -4937(ix)
	push	hl
	ld	l, -4942(ix)
	ld	h, -4941(ix)
	ld	b, l
	pop	hl
__shift_8138:
	ld	a, b
	or	a, a
	jp	z, __sdone_1225
	add	hl, hl
	djnz	__shift_8138
__sdone_1225:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4946(ix), l
	ld	-4945(ix), h
	ld	l, -4946(ix)
	ld	h, -4945(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4948(ix), l
	ld	-4947(ix), h
	ld	l, -4948(ix)
	ld	h, -4947(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90495
	ld	hl, #0
	jp	__cmp_e_17540
__cmp_t_90495:
	ld	hl, #1
__cmp_e_17540:
	dec	sp
	dec	sp
	ld	-4950(ix), l
	ld	-4949(ix), h
	ld	l, -4950(ix)
	ld	h, -4949(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_36421
	ld	hl, #0
	jp	__cmp_e_68972
__cmp_t_36421:
	ld	hl, #1
__cmp_e_68972:
	dec	sp
	dec	sp
	ld	-4952(ix), l
	ld	-4951(ix), h
	jp	__xcc_L1056
__xcc_L1054:
	ld	hl, #1
	ld	-4952(ix), l
	ld	-4951(ix), h
__xcc_L1056:
	ld	l, -4952(ix)
	ld	h, -4951(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1051
	jp	__xcc_L1052
__xcc_L1051:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4954(ix), l
	ld	-4953(ix), h
	ld	l, -4954(ix)
	ld	h, -4953(ix)
	dec	sp
	dec	sp
	ld	-4956(ix), l
	ld	-4955(ix), h
	jp	__xcc_L1053
__xcc_L1052:
	ld	hl, #1
	ld	-4956(ix), l
	ld	-4955(ix), h
__xcc_L1053:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-4958(ix), l
	ld	-4957(ix), h
	.globl __mul16
	ld	l, -4958(ix)
	ld	h, -4957(ix)
	push	hl
	ld	l, -4956(ix)
	ld	h, -4955(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4960(ix), l
	ld	-4959(ix), h
	ld	l, -4960(ix)
	ld	h, -4959(ix)
	push	hl
	ld	l, -4918(ix)
	ld	h, -4917(ix)
	push	hl
	ld	l, -4892(ix)
	ld	h, -4891(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1057
	dec	sp
	dec	sp
	ld	-4962(ix), l
	ld	-4961(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4964(ix), l
	ld	-4963(ix), h
	ld	l, -4964(ix)
	ld	h, -4963(ix)
	dec	sp
	dec	sp
	ld	-4966(ix), l
	ld	-4965(ix), h
	ld	l, -4966(ix)
	ld	h, -4965(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87226
	ld	hl, #0
	jp	__cmp_e_40634
__cmp_t_87226:
	ld	hl, #1
__cmp_e_40634:
	dec	sp
	dec	sp
	ld	-4968(ix), l
	ld	-4967(ix), h
	ld	l, -4968(ix)
	ld	h, -4967(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34158
	ld	hl, #0
	jp	__cmp_e_65725
__cmp_t_34158:
	ld	hl, #1
__cmp_e_65725:
	dec	sp
	dec	sp
	ld	-4970(ix), l
	ld	-4969(ix), h
	ld	l, -4970(ix)
	ld	h, -4969(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1061
	jp	__xcc_L1062
__xcc_L1062:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4972(ix), l
	ld	-4971(ix), h
	ld	l, -4972(ix)
	ld	h, -4971(ix)
	dec	sp
	dec	sp
	ld	-4974(ix), l
	ld	-4973(ix), h
	ld	l, -4974(ix)
	ld	h, -4973(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4976(ix), l
	ld	-4975(ix), h
	ld	l, -4976(ix)
	ld	h, -4975(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20356
	ld	hl, #0
	jp	__cmp_e_90952
__cmp_t_20356:
	ld	hl, #1
__cmp_e_90952:
	dec	sp
	dec	sp
	ld	-4978(ix), l
	ld	-4977(ix), h
	ld	l, -4978(ix)
	ld	h, -4977(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_77645
	ld	hl, #0
	jp	__cmp_e_85472
__cmp_t_77645:
	ld	hl, #1
__cmp_e_85472:
	dec	sp
	dec	sp
	ld	-4980(ix), l
	ld	-4979(ix), h
	jp	__xcc_L1063
__xcc_L1061:
	ld	hl, #1
	ld	-4980(ix), l
	ld	-4979(ix), h
__xcc_L1063:
	ld	l, -4980(ix)
	ld	h, -4979(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1058
	jp	__xcc_L1059
__xcc_L1058:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4982(ix), l
	ld	-4981(ix), h
	ld	l, -4982(ix)
	ld	h, -4981(ix)
	dec	sp
	dec	sp
	ld	-4984(ix), l
	ld	-4983(ix), h
	jp	__xcc_L1060
__xcc_L1059:
	ld	hl, #1
	ld	-4984(ix), l
	ld	-4983(ix), h
__xcc_L1060:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-4986(ix), l
	ld	-4985(ix), h
	.globl __mul16
	ld	l, -4986(ix)
	ld	h, -4985(ix)
	push	hl
	ld	l, -4984(ix)
	ld	h, -4983(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4988(ix), l
	ld	-4987(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-4990(ix), l
	ld	-4989(ix), h
	ld	l, -4990(ix)
	ld	h, -4989(ix)
	dec	sp
	dec	sp
	ld	-4992(ix), l
	ld	-4991(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-4996(ix), l
	ld	-4995(ix), h
	ld	l, -4992(ix)
	ld	h, -4991(ix)
	push	hl
	ld	l, -4996(ix)
	ld	h, -4995(ix)
	ld	b, l
	pop	hl
__shift_3446:
	ld	a, b
	or	a, a
	jp	z, __sdone_3339
	add	hl, hl
	djnz	__shift_3446
__sdone_3339:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5000(ix), l
	ld	-4999(ix), h
	ld	l, -5000(ix)
	ld	h, -4999(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_68111
	ld	hl, #0
	jp	__cmp_e_75883
__cmp_t_68111:
	ld	hl, #1
__cmp_e_75883:
	dec	sp
	dec	sp
	ld	-5002(ix), l
	ld	-5001(ix), h
	ld	l, -5002(ix)
	ld	h, -5001(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17603
	ld	hl, #0
	jp	__cmp_e_47661
__cmp_t_17603:
	ld	hl, #1
__cmp_e_47661:
	dec	sp
	dec	sp
	ld	-5004(ix), l
	ld	-5003(ix), h
	ld	l, -5004(ix)
	ld	h, -5003(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1067
	jp	__xcc_L1068
__xcc_L1068:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5006(ix), l
	ld	-5005(ix), h
	ld	l, -5006(ix)
	ld	h, -5005(ix)
	dec	sp
	dec	sp
	ld	-5008(ix), l
	ld	-5007(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5012(ix), l
	ld	-5011(ix), h
	ld	l, -5008(ix)
	ld	h, -5007(ix)
	push	hl
	ld	l, -5012(ix)
	ld	h, -5011(ix)
	ld	b, l
	pop	hl
__shift_5825:
	ld	a, b
	or	a, a
	jp	z, __sdone_7542
	add	hl, hl
	djnz	__shift_5825
__sdone_7542:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5016(ix), l
	ld	-5015(ix), h
	ld	l, -5016(ix)
	ld	h, -5015(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5018(ix), l
	ld	-5017(ix), h
	ld	l, -5018(ix)
	ld	h, -5017(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_70216
	ld	hl, #0
	jp	__cmp_e_82339
__cmp_t_70216:
	ld	hl, #1
__cmp_e_82339:
	dec	sp
	dec	sp
	ld	-5020(ix), l
	ld	-5019(ix), h
	ld	l, -5020(ix)
	ld	h, -5019(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79174
	ld	hl, #0
	jp	__cmp_e_35344
__cmp_t_79174:
	ld	hl, #1
__cmp_e_35344:
	dec	sp
	dec	sp
	ld	-5022(ix), l
	ld	-5021(ix), h
	jp	__xcc_L1069
__xcc_L1067:
	ld	hl, #1
	ld	-5022(ix), l
	ld	-5021(ix), h
__xcc_L1069:
	ld	l, -5022(ix)
	ld	h, -5021(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1064
	jp	__xcc_L1065
__xcc_L1064:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5024(ix), l
	ld	-5023(ix), h
	ld	l, -5024(ix)
	ld	h, -5023(ix)
	dec	sp
	dec	sp
	ld	-5026(ix), l
	ld	-5025(ix), h
	jp	__xcc_L1066
__xcc_L1065:
	ld	hl, #1
	ld	-5026(ix), l
	ld	-5025(ix), h
__xcc_L1066:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-5028(ix), l
	ld	-5027(ix), h
	.globl __mul16
	ld	l, -5028(ix)
	ld	h, -5027(ix)
	push	hl
	ld	l, -5026(ix)
	ld	h, -5025(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5030(ix), l
	ld	-5029(ix), h
	ld	l, -5030(ix)
	ld	h, -5029(ix)
	push	hl
	ld	l, -4988(ix)
	ld	h, -4987(ix)
	push	hl
	ld	l, -4962(ix)
	ld	h, -4961(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1042:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1041
	jp	__xcc_L1043
__xcc_L1043:
__xcc_L1070:
	ld	hl, #__str_1073
	dec	sp
	dec	sp
	ld	-5032(ix), l
	ld	-5031(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5034(ix), l
	ld	-5033(ix), h
	ld	l, -5034(ix)
	ld	h, -5033(ix)
	dec	sp
	dec	sp
	ld	-5036(ix), l
	ld	-5035(ix), h
	ld	l, -5036(ix)
	ld	h, -5035(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60596
	ld	hl, #0
	jp	__cmp_e_7330
__cmp_t_60596:
	ld	hl, #1
__cmp_e_7330:
	dec	sp
	dec	sp
	ld	-5038(ix), l
	ld	-5037(ix), h
	ld	l, -5038(ix)
	ld	h, -5037(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_46267
	ld	hl, #0
	jp	__cmp_e_29294
__cmp_t_46267:
	ld	hl, #1
__cmp_e_29294:
	dec	sp
	dec	sp
	ld	-5040(ix), l
	ld	-5039(ix), h
	ld	l, -5040(ix)
	ld	h, -5039(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1077
	jp	__xcc_L1078
__xcc_L1078:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5042(ix), l
	ld	-5041(ix), h
	ld	l, -5042(ix)
	ld	h, -5041(ix)
	dec	sp
	dec	sp
	ld	-5044(ix), l
	ld	-5043(ix), h
	ld	l, -5044(ix)
	ld	h, -5043(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5046(ix), l
	ld	-5045(ix), h
	ld	l, -5046(ix)
	ld	h, -5045(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_42013
	ld	hl, #0
	jp	__cmp_e_60757
__cmp_t_42013:
	ld	hl, #1
__cmp_e_60757:
	dec	sp
	dec	sp
	ld	-5048(ix), l
	ld	-5047(ix), h
	ld	l, -5048(ix)
	ld	h, -5047(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80519
	ld	hl, #0
	jp	__cmp_e_48860
__cmp_t_80519:
	ld	hl, #1
__cmp_e_48860:
	dec	sp
	dec	sp
	ld	-5050(ix), l
	ld	-5049(ix), h
	jp	__xcc_L1079
__xcc_L1077:
	ld	hl, #1
	ld	-5050(ix), l
	ld	-5049(ix), h
__xcc_L1079:
	ld	l, -5050(ix)
	ld	h, -5049(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1074
	jp	__xcc_L1075
__xcc_L1074:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5052(ix), l
	ld	-5051(ix), h
	ld	l, -5052(ix)
	ld	h, -5051(ix)
	dec	sp
	dec	sp
	ld	-5054(ix), l
	ld	-5053(ix), h
	jp	__xcc_L1076
__xcc_L1075:
	ld	hl, #1
	ld	-5054(ix), l
	ld	-5053(ix), h
__xcc_L1076:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5056(ix), l
	ld	-5055(ix), h
	.globl __mul16
	ld	l, -5056(ix)
	ld	h, -5055(ix)
	push	hl
	ld	l, -5054(ix)
	ld	h, -5053(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5058(ix), l
	ld	-5057(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5060(ix), l
	ld	-5059(ix), h
	ld	l, -5060(ix)
	ld	h, -5059(ix)
	dec	sp
	dec	sp
	ld	-5062(ix), l
	ld	-5061(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5070(ix), l
	ld	-5069(ix), h
	ld	l, -5062(ix)
	ld	h, -5061(ix)
	push	hl
	ld	l, -5070(ix)
	ld	h, -5069(ix)
	ld	b, l
	pop	hl
__shift_4650:
	ld	a, b
	or	a, a
	jp	z, __sdone_3292
	add	hl, hl
	djnz	__shift_4650
__sdone_3292:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5078(ix), l
	ld	-5077(ix), h
	ld	l, -5078(ix)
	ld	h, -5077(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_17832
	ld	hl, #0
	jp	__cmp_e_81876
__cmp_t_17832:
	ld	hl, #1
__cmp_e_81876:
	dec	sp
	dec	sp
	ld	-5080(ix), l
	ld	-5079(ix), h
	ld	l, -5080(ix)
	ld	h, -5079(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_90279
	ld	hl, #0
	jp	__cmp_e_51990
__cmp_t_90279:
	ld	hl, #1
__cmp_e_51990:
	dec	sp
	dec	sp
	ld	-5082(ix), l
	ld	-5081(ix), h
	ld	l, -5082(ix)
	ld	h, -5081(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1083
	jp	__xcc_L1084
__xcc_L1084:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5084(ix), l
	ld	-5083(ix), h
	ld	l, -5084(ix)
	ld	h, -5083(ix)
	dec	sp
	dec	sp
	ld	-5086(ix), l
	ld	-5085(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5094(ix), l
	ld	-5093(ix), h
	ld	l, -5086(ix)
	ld	h, -5085(ix)
	push	hl
	ld	l, -5094(ix)
	ld	h, -5093(ix)
	ld	b, l
	pop	hl
__shift_3953:
	ld	a, b
	or	a, a
	jp	z, __sdone_635
	add	hl, hl
	djnz	__shift_3953
__sdone_635:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5102(ix), l
	ld	-5101(ix), h
	ld	l, -5102(ix)
	ld	h, -5101(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5104(ix), l
	ld	-5103(ix), h
	ld	l, -5104(ix)
	ld	h, -5103(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59295
	ld	hl, #0
	jp	__cmp_e_41598
__cmp_t_59295:
	ld	hl, #1
__cmp_e_41598:
	dec	sp
	dec	sp
	ld	-5106(ix), l
	ld	-5105(ix), h
	ld	l, -5106(ix)
	ld	h, -5105(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96107
	ld	hl, #0
	jp	__cmp_e_52741
__cmp_t_96107:
	ld	hl, #1
__cmp_e_52741:
	dec	sp
	dec	sp
	ld	-5108(ix), l
	ld	-5107(ix), h
	jp	__xcc_L1085
__xcc_L1083:
	ld	hl, #1
	ld	-5108(ix), l
	ld	-5107(ix), h
__xcc_L1085:
	ld	l, -5108(ix)
	ld	h, -5107(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1080
	jp	__xcc_L1081
__xcc_L1080:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5110(ix), l
	ld	-5109(ix), h
	ld	l, -5110(ix)
	ld	h, -5109(ix)
	dec	sp
	dec	sp
	ld	-5112(ix), l
	ld	-5111(ix), h
	jp	__xcc_L1082
__xcc_L1081:
	ld	hl, #1
	ld	-5112(ix), l
	ld	-5111(ix), h
__xcc_L1082:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-5114(ix), l
	ld	-5113(ix), h
	.globl __mul16
	ld	l, -5114(ix)
	ld	h, -5113(ix)
	push	hl
	ld	l, -5112(ix)
	ld	h, -5111(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5116(ix), l
	ld	-5115(ix), h
	ld	l, -5116(ix)
	ld	h, -5115(ix)
	push	hl
	ld	l, -5058(ix)
	ld	h, -5057(ix)
	push	hl
	ld	l, -5032(ix)
	ld	h, -5031(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1086
	dec	sp
	dec	sp
	ld	-5118(ix), l
	ld	-5117(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5120(ix), l
	ld	-5119(ix), h
	ld	l, -5120(ix)
	ld	h, -5119(ix)
	dec	sp
	dec	sp
	ld	-5122(ix), l
	ld	-5121(ix), h
	ld	l, -5122(ix)
	ld	h, -5121(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_14937
	ld	hl, #0
	jp	__cmp_e_80570
__cmp_t_14937:
	ld	hl, #1
__cmp_e_80570:
	dec	sp
	dec	sp
	ld	-5124(ix), l
	ld	-5123(ix), h
	ld	l, -5124(ix)
	ld	h, -5123(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44976
	ld	hl, #0
	jp	__cmp_e_32540
__cmp_t_44976:
	ld	hl, #1
__cmp_e_32540:
	dec	sp
	dec	sp
	ld	-5126(ix), l
	ld	-5125(ix), h
	ld	l, -5126(ix)
	ld	h, -5125(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1090
	jp	__xcc_L1091
__xcc_L1091:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5128(ix), l
	ld	-5127(ix), h
	ld	l, -5128(ix)
	ld	h, -5127(ix)
	dec	sp
	dec	sp
	ld	-5130(ix), l
	ld	-5129(ix), h
	ld	l, -5130(ix)
	ld	h, -5129(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5132(ix), l
	ld	-5131(ix), h
	ld	l, -5132(ix)
	ld	h, -5131(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_44584
	ld	hl, #0
	jp	__cmp_e_801
__cmp_t_44584:
	ld	hl, #1
__cmp_e_801:
	dec	sp
	dec	sp
	ld	-5134(ix), l
	ld	-5133(ix), h
	ld	l, -5134(ix)
	ld	h, -5133(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_20083
	ld	hl, #0
	jp	__cmp_e_14800
__cmp_t_20083:
	ld	hl, #1
__cmp_e_14800:
	dec	sp
	dec	sp
	ld	-5136(ix), l
	ld	-5135(ix), h
	jp	__xcc_L1092
__xcc_L1090:
	ld	hl, #1
	ld	-5136(ix), l
	ld	-5135(ix), h
__xcc_L1092:
	ld	l, -5136(ix)
	ld	h, -5135(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1087
	jp	__xcc_L1088
__xcc_L1087:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5138(ix), l
	ld	-5137(ix), h
	ld	l, -5138(ix)
	ld	h, -5137(ix)
	dec	sp
	dec	sp
	ld	-5140(ix), l
	ld	-5139(ix), h
	jp	__xcc_L1089
__xcc_L1088:
	ld	hl, #1
	ld	-5140(ix), l
	ld	-5139(ix), h
__xcc_L1089:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5142(ix), l
	ld	-5141(ix), h
	.globl __mul16
	ld	l, -5142(ix)
	ld	h, -5141(ix)
	push	hl
	ld	l, -5140(ix)
	ld	h, -5139(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5144(ix), l
	ld	-5143(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5146(ix), l
	ld	-5145(ix), h
	ld	l, -5146(ix)
	ld	h, -5145(ix)
	dec	sp
	dec	sp
	ld	-5148(ix), l
	ld	-5147(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5156(ix), l
	ld	-5155(ix), h
	ld	l, -5148(ix)
	ld	h, -5147(ix)
	push	hl
	ld	l, -5156(ix)
	ld	h, -5155(ix)
	ld	b, l
	pop	hl
__shift_9492:
	ld	a, b
	or	a, a
	jp	z, __sdone_5609
	add	hl, hl
	djnz	__shift_9492
__sdone_5609:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5164(ix), l
	ld	-5163(ix), h
	ld	l, -5164(ix)
	ld	h, -5163(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_66496
	ld	hl, #0
	jp	__cmp_e_76440
__cmp_t_66496:
	ld	hl, #1
__cmp_e_76440:
	dec	sp
	dec	sp
	ld	-5166(ix), l
	ld	-5165(ix), h
	ld	l, -5166(ix)
	ld	h, -5165(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22939
	ld	hl, #0
	jp	__cmp_e_12763
__cmp_t_22939:
	ld	hl, #1
__cmp_e_12763:
	dec	sp
	dec	sp
	ld	-5168(ix), l
	ld	-5167(ix), h
	ld	l, -5168(ix)
	ld	h, -5167(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1096
	jp	__xcc_L1097
__xcc_L1097:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5170(ix), l
	ld	-5169(ix), h
	ld	l, -5170(ix)
	ld	h, -5169(ix)
	dec	sp
	dec	sp
	ld	-5172(ix), l
	ld	-5171(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5180(ix), l
	ld	-5179(ix), h
	ld	l, -5172(ix)
	ld	h, -5171(ix)
	push	hl
	ld	l, -5180(ix)
	ld	h, -5179(ix)
	ld	b, l
	pop	hl
__shift_5735:
	ld	a, b
	or	a, a
	jp	z, __sdone_1304
	add	hl, hl
	djnz	__shift_5735
__sdone_1304:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5188(ix), l
	ld	-5187(ix), h
	ld	l, -5188(ix)
	ld	h, -5187(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5190(ix), l
	ld	-5189(ix), h
	ld	l, -5190(ix)
	ld	h, -5189(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89873
	ld	hl, #0
	jp	__cmp_e_2606
__cmp_t_89873:
	ld	hl, #1
__cmp_e_2606:
	dec	sp
	dec	sp
	ld	-5192(ix), l
	ld	-5191(ix), h
	ld	l, -5192(ix)
	ld	h, -5191(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30164
	ld	hl, #0
	jp	__cmp_e_84523
__cmp_t_30164:
	ld	hl, #1
__cmp_e_84523:
	dec	sp
	dec	sp
	ld	-5194(ix), l
	ld	-5193(ix), h
	jp	__xcc_L1098
__xcc_L1096:
	ld	hl, #1
	ld	-5194(ix), l
	ld	-5193(ix), h
__xcc_L1098:
	ld	l, -5194(ix)
	ld	h, -5193(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1093
	jp	__xcc_L1094
__xcc_L1093:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5196(ix), l
	ld	-5195(ix), h
	ld	l, -5196(ix)
	ld	h, -5195(ix)
	dec	sp
	dec	sp
	ld	-5198(ix), l
	ld	-5197(ix), h
	jp	__xcc_L1095
__xcc_L1094:
	ld	hl, #1
	ld	-5198(ix), l
	ld	-5197(ix), h
__xcc_L1095:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-5200(ix), l
	ld	-5199(ix), h
	.globl __mul16
	ld	l, -5200(ix)
	ld	h, -5199(ix)
	push	hl
	ld	l, -5198(ix)
	ld	h, -5197(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5202(ix), l
	ld	-5201(ix), h
	ld	l, -5202(ix)
	ld	h, -5201(ix)
	push	hl
	ld	l, -5144(ix)
	ld	h, -5143(ix)
	push	hl
	ld	l, -5118(ix)
	ld	h, -5117(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1071:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1070
	jp	__xcc_L1072
__xcc_L1072:
__xcc_L981:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L980
	jp	__xcc_L982
__xcc_L982:
__xcc_L1099:
__xcc_L1102:
	ld	hl, #__str_1105
	dec	sp
	dec	sp
	ld	-5204(ix), l
	ld	-5203(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5206(ix), l
	ld	-5205(ix), h
	ld	l, -5206(ix)
	ld	h, -5205(ix)
	dec	sp
	dec	sp
	ld	-5208(ix), l
	ld	-5207(ix), h
	ld	l, -5208(ix)
	ld	h, -5207(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_52251
	ld	hl, #0
	jp	__cmp_e_64349
__cmp_t_52251:
	ld	hl, #1
__cmp_e_64349:
	dec	sp
	dec	sp
	ld	-5210(ix), l
	ld	-5209(ix), h
	ld	l, -5210(ix)
	ld	h, -5209(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_82751
	ld	hl, #0
	jp	__cmp_e_42530
__cmp_t_82751:
	ld	hl, #1
__cmp_e_42530:
	dec	sp
	dec	sp
	ld	-5212(ix), l
	ld	-5211(ix), h
	ld	l, -5212(ix)
	ld	h, -5211(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1109
	jp	__xcc_L1110
__xcc_L1110:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5214(ix), l
	ld	-5213(ix), h
	ld	l, -5214(ix)
	ld	h, -5213(ix)
	dec	sp
	dec	sp
	ld	-5216(ix), l
	ld	-5215(ix), h
	ld	l, -5216(ix)
	ld	h, -5215(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5218(ix), l
	ld	-5217(ix), h
	ld	l, -5218(ix)
	ld	h, -5217(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16339
	ld	hl, #0
	jp	__cmp_e_46704
__cmp_t_16339:
	ld	hl, #1
__cmp_e_46704:
	dec	sp
	dec	sp
	ld	-5220(ix), l
	ld	-5219(ix), h
	ld	l, -5220(ix)
	ld	h, -5219(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_53165
	ld	hl, #0
	jp	__cmp_e_91986
__cmp_t_53165:
	ld	hl, #1
__cmp_e_91986:
	dec	sp
	dec	sp
	ld	-5222(ix), l
	ld	-5221(ix), h
	jp	__xcc_L1111
__xcc_L1109:
	ld	hl, #1
	ld	-5222(ix), l
	ld	-5221(ix), h
__xcc_L1111:
	ld	l, -5222(ix)
	ld	h, -5221(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1106
	jp	__xcc_L1107
__xcc_L1106:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5224(ix), l
	ld	-5223(ix), h
	ld	l, -5224(ix)
	ld	h, -5223(ix)
	dec	sp
	dec	sp
	ld	-5226(ix), l
	ld	-5225(ix), h
	jp	__xcc_L1108
__xcc_L1107:
	ld	hl, #1
	ld	-5226(ix), l
	ld	-5225(ix), h
__xcc_L1108:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5228(ix), l
	ld	-5227(ix), h
	.globl __mul16
	ld	l, -5228(ix)
	ld	h, -5227(ix)
	push	hl
	ld	l, -5226(ix)
	ld	h, -5225(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5230(ix), l
	ld	-5229(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5232(ix), l
	ld	-5231(ix), h
	ld	l, -5232(ix)
	ld	h, -5231(ix)
	dec	sp
	dec	sp
	ld	-5234(ix), l
	ld	-5233(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5236(ix), l
	ld	-5235(ix), h
	ld	l, -5234(ix)
	ld	h, -5233(ix)
	push	hl
	ld	l, -5236(ix)
	ld	h, -5235(ix)
	ld	b, l
	pop	hl
__shift_8302:
	ld	a, b
	or	a, a
	jp	z, __sdone_5625
	add	hl, hl
	djnz	__shift_8302
__sdone_5625:
	dec	sp
	dec	sp
	ld	-5238(ix), l
	ld	-5237(ix), h
	ld	l, -5238(ix)
	ld	h, -5237(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_61079
	ld	hl, #0
	jp	__cmp_e_19591
__cmp_t_61079:
	ld	hl, #1
__cmp_e_19591:
	dec	sp
	dec	sp
	ld	-5240(ix), l
	ld	-5239(ix), h
	ld	l, -5240(ix)
	ld	h, -5239(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_62547
	ld	hl, #0
	jp	__cmp_e_22407
__cmp_t_62547:
	ld	hl, #1
__cmp_e_22407:
	dec	sp
	dec	sp
	ld	-5242(ix), l
	ld	-5241(ix), h
	ld	l, -5242(ix)
	ld	h, -5241(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1115
	jp	__xcc_L1116
__xcc_L1116:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5244(ix), l
	ld	-5243(ix), h
	ld	l, -5244(ix)
	ld	h, -5243(ix)
	dec	sp
	dec	sp
	ld	-5246(ix), l
	ld	-5245(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5248(ix), l
	ld	-5247(ix), h
	ld	l, -5246(ix)
	ld	h, -5245(ix)
	push	hl
	ld	l, -5248(ix)
	ld	h, -5247(ix)
	ld	b, l
	pop	hl
__shift_8484:
	ld	a, b
	or	a, a
	jp	z, __sdone_7131
	add	hl, hl
	djnz	__shift_8484
__sdone_7131:
	dec	sp
	dec	sp
	ld	-5250(ix), l
	ld	-5249(ix), h
	ld	l, -5250(ix)
	ld	h, -5249(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5252(ix), l
	ld	-5251(ix), h
	ld	l, -5252(ix)
	ld	h, -5251(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39561
	ld	hl, #0
	jp	__cmp_e_4919
__cmp_t_39561:
	ld	hl, #1
__cmp_e_4919:
	dec	sp
	dec	sp
	ld	-5254(ix), l
	ld	-5253(ix), h
	ld	l, -5254(ix)
	ld	h, -5253(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_21931
	ld	hl, #0
	jp	__cmp_e_39053
__cmp_t_21931:
	ld	hl, #1
__cmp_e_39053:
	dec	sp
	dec	sp
	ld	-5256(ix), l
	ld	-5255(ix), h
	jp	__xcc_L1117
__xcc_L1115:
	ld	hl, #1
	ld	-5256(ix), l
	ld	-5255(ix), h
__xcc_L1117:
	ld	l, -5256(ix)
	ld	h, -5255(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1112
	jp	__xcc_L1113
__xcc_L1112:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5258(ix), l
	ld	-5257(ix), h
	ld	l, -5258(ix)
	ld	h, -5257(ix)
	dec	sp
	dec	sp
	ld	-5260(ix), l
	ld	-5259(ix), h
	jp	__xcc_L1114
__xcc_L1113:
	ld	hl, #1
	ld	-5260(ix), l
	ld	-5259(ix), h
__xcc_L1114:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5262(ix), l
	ld	-5261(ix), h
	.globl __mul16
	ld	l, -5262(ix)
	ld	h, -5261(ix)
	push	hl
	ld	l, -5260(ix)
	ld	h, -5259(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5264(ix), l
	ld	-5263(ix), h
	ld	l, -5264(ix)
	ld	h, -5263(ix)
	push	hl
	ld	l, -5230(ix)
	ld	h, -5229(ix)
	push	hl
	ld	l, -5204(ix)
	ld	h, -5203(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1118
	dec	sp
	dec	sp
	ld	-5266(ix), l
	ld	-5265(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5268(ix), l
	ld	-5267(ix), h
	ld	l, -5268(ix)
	ld	h, -5267(ix)
	dec	sp
	dec	sp
	ld	-5270(ix), l
	ld	-5269(ix), h
	ld	l, -5270(ix)
	ld	h, -5269(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20528
	ld	hl, #0
	jp	__cmp_e_88427
__cmp_t_20528:
	ld	hl, #1
__cmp_e_88427:
	dec	sp
	dec	sp
	ld	-5272(ix), l
	ld	-5271(ix), h
	ld	l, -5272(ix)
	ld	h, -5271(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15494
	ld	hl, #0
	jp	__cmp_e_59819
__cmp_t_15494:
	ld	hl, #1
__cmp_e_59819:
	dec	sp
	dec	sp
	ld	-5274(ix), l
	ld	-5273(ix), h
	ld	l, -5274(ix)
	ld	h, -5273(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1122
	jp	__xcc_L1123
__xcc_L1123:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5276(ix), l
	ld	-5275(ix), h
	ld	l, -5276(ix)
	ld	h, -5275(ix)
	dec	sp
	dec	sp
	ld	-5278(ix), l
	ld	-5277(ix), h
	ld	l, -5278(ix)
	ld	h, -5277(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5280(ix), l
	ld	-5279(ix), h
	ld	l, -5280(ix)
	ld	h, -5279(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_17543
	ld	hl, #0
	jp	__cmp_e_37581
__cmp_t_17543:
	ld	hl, #1
__cmp_e_37581:
	dec	sp
	dec	sp
	ld	-5282(ix), l
	ld	-5281(ix), h
	ld	l, -5282(ix)
	ld	h, -5281(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41123
	ld	hl, #0
	jp	__cmp_e_23768
__cmp_t_41123:
	ld	hl, #1
__cmp_e_23768:
	dec	sp
	dec	sp
	ld	-5284(ix), l
	ld	-5283(ix), h
	jp	__xcc_L1124
__xcc_L1122:
	ld	hl, #1
	ld	-5284(ix), l
	ld	-5283(ix), h
__xcc_L1124:
	ld	l, -5284(ix)
	ld	h, -5283(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1119
	jp	__xcc_L1120
__xcc_L1119:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5286(ix), l
	ld	-5285(ix), h
	ld	l, -5286(ix)
	ld	h, -5285(ix)
	dec	sp
	dec	sp
	ld	-5288(ix), l
	ld	-5287(ix), h
	jp	__xcc_L1121
__xcc_L1120:
	ld	hl, #1
	ld	-5288(ix), l
	ld	-5287(ix), h
__xcc_L1121:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5290(ix), l
	ld	-5289(ix), h
	.globl __mul16
	ld	l, -5290(ix)
	ld	h, -5289(ix)
	push	hl
	ld	l, -5288(ix)
	ld	h, -5287(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5292(ix), l
	ld	-5291(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5294(ix), l
	ld	-5293(ix), h
	ld	l, -5294(ix)
	ld	h, -5293(ix)
	dec	sp
	dec	sp
	ld	-5296(ix), l
	ld	-5295(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5298(ix), l
	ld	-5297(ix), h
	ld	l, -5296(ix)
	ld	h, -5295(ix)
	push	hl
	ld	l, -5298(ix)
	ld	h, -5297(ix)
	ld	b, l
	pop	hl
__shift_187:
	ld	a, b
	or	a, a
	jp	z, __sdone_7639
	add	hl, hl
	djnz	__shift_187
__sdone_7639:
	dec	sp
	dec	sp
	ld	-5300(ix), l
	ld	-5299(ix), h
	ld	l, -5300(ix)
	ld	h, -5299(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_24643
	ld	hl, #0
	jp	__cmp_e_92438
__cmp_t_24643:
	ld	hl, #1
__cmp_e_92438:
	dec	sp
	dec	sp
	ld	-5302(ix), l
	ld	-5301(ix), h
	ld	l, -5302(ix)
	ld	h, -5301(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51988
	ld	hl, #0
	jp	__cmp_e_7394
__cmp_t_51988:
	ld	hl, #1
__cmp_e_7394:
	dec	sp
	dec	sp
	ld	-5304(ix), l
	ld	-5303(ix), h
	ld	l, -5304(ix)
	ld	h, -5303(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1128
	jp	__xcc_L1129
__xcc_L1129:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5306(ix), l
	ld	-5305(ix), h
	ld	l, -5306(ix)
	ld	h, -5305(ix)
	dec	sp
	dec	sp
	ld	-5308(ix), l
	ld	-5307(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5310(ix), l
	ld	-5309(ix), h
	ld	l, -5308(ix)
	ld	h, -5307(ix)
	push	hl
	ld	l, -5310(ix)
	ld	h, -5309(ix)
	ld	b, l
	pop	hl
__shift_1320:
	ld	a, b
	or	a, a
	jp	z, __sdone_4680
	add	hl, hl
	djnz	__shift_1320
__sdone_4680:
	dec	sp
	dec	sp
	ld	-5312(ix), l
	ld	-5311(ix), h
	ld	l, -5312(ix)
	ld	h, -5311(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5314(ix), l
	ld	-5313(ix), h
	ld	l, -5314(ix)
	ld	h, -5313(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_54098
	ld	hl, #0
	jp	__cmp_e_4486
__cmp_t_54098:
	ld	hl, #1
__cmp_e_4486:
	dec	sp
	dec	sp
	ld	-5316(ix), l
	ld	-5315(ix), h
	ld	l, -5316(ix)
	ld	h, -5315(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93018
	ld	hl, #0
	jp	__cmp_e_58752
__cmp_t_93018:
	ld	hl, #1
__cmp_e_58752:
	dec	sp
	dec	sp
	ld	-5318(ix), l
	ld	-5317(ix), h
	jp	__xcc_L1130
__xcc_L1128:
	ld	hl, #1
	ld	-5318(ix), l
	ld	-5317(ix), h
__xcc_L1130:
	ld	l, -5318(ix)
	ld	h, -5317(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1125
	jp	__xcc_L1126
__xcc_L1125:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5320(ix), l
	ld	-5319(ix), h
	ld	l, -5320(ix)
	ld	h, -5319(ix)
	dec	sp
	dec	sp
	ld	-5322(ix), l
	ld	-5321(ix), h
	jp	__xcc_L1127
__xcc_L1126:
	ld	hl, #1
	ld	-5322(ix), l
	ld	-5321(ix), h
__xcc_L1127:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5324(ix), l
	ld	-5323(ix), h
	.globl __mul16
	ld	l, -5324(ix)
	ld	h, -5323(ix)
	push	hl
	ld	l, -5322(ix)
	ld	h, -5321(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5326(ix), l
	ld	-5325(ix), h
	ld	l, -5326(ix)
	ld	h, -5325(ix)
	push	hl
	ld	l, -5292(ix)
	ld	h, -5291(ix)
	push	hl
	ld	l, -5266(ix)
	ld	h, -5265(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1103:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1102
	jp	__xcc_L1104
__xcc_L1104:
__xcc_L1131:
	ld	hl, #__str_1134
	dec	sp
	dec	sp
	ld	-5328(ix), l
	ld	-5327(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5330(ix), l
	ld	-5329(ix), h
	ld	l, -5330(ix)
	ld	h, -5329(ix)
	dec	sp
	dec	sp
	ld	-5332(ix), l
	ld	-5331(ix), h
	ld	l, -5332(ix)
	ld	h, -5331(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_86463
	ld	hl, #0
	jp	__cmp_e_54098
__cmp_t_86463:
	ld	hl, #1
__cmp_e_54098:
	dec	sp
	dec	sp
	ld	-5334(ix), l
	ld	-5333(ix), h
	ld	l, -5334(ix)
	ld	h, -5333(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_94695
	ld	hl, #0
	jp	__cmp_e_49010
__cmp_t_94695:
	ld	hl, #1
__cmp_e_49010:
	dec	sp
	dec	sp
	ld	-5336(ix), l
	ld	-5335(ix), h
	ld	l, -5336(ix)
	ld	h, -5335(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1138
	jp	__xcc_L1139
__xcc_L1139:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5338(ix), l
	ld	-5337(ix), h
	ld	l, -5338(ix)
	ld	h, -5337(ix)
	dec	sp
	dec	sp
	ld	-5340(ix), l
	ld	-5339(ix), h
	ld	l, -5340(ix)
	ld	h, -5339(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5342(ix), l
	ld	-5341(ix), h
	ld	l, -5342(ix)
	ld	h, -5341(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_76505
	ld	hl, #0
	jp	__cmp_e_63179
__cmp_t_76505:
	ld	hl, #1
__cmp_e_63179:
	dec	sp
	dec	sp
	ld	-5344(ix), l
	ld	-5343(ix), h
	ld	l, -5344(ix)
	ld	h, -5343(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_56142
	ld	hl, #0
	jp	__cmp_e_16066
__cmp_t_56142:
	ld	hl, #1
__cmp_e_16066:
	dec	sp
	dec	sp
	ld	-5346(ix), l
	ld	-5345(ix), h
	jp	__xcc_L1140
__xcc_L1138:
	ld	hl, #1
	ld	-5346(ix), l
	ld	-5345(ix), h
__xcc_L1140:
	ld	l, -5346(ix)
	ld	h, -5345(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1135
	jp	__xcc_L1136
__xcc_L1135:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5348(ix), l
	ld	-5347(ix), h
	ld	l, -5348(ix)
	ld	h, -5347(ix)
	dec	sp
	dec	sp
	ld	-5350(ix), l
	ld	-5349(ix), h
	jp	__xcc_L1137
__xcc_L1136:
	ld	hl, #1
	ld	-5350(ix), l
	ld	-5349(ix), h
__xcc_L1137:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5352(ix), l
	ld	-5351(ix), h
	.globl __mul16
	ld	l, -5352(ix)
	ld	h, -5351(ix)
	push	hl
	ld	l, -5350(ix)
	ld	h, -5349(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5354(ix), l
	ld	-5353(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5356(ix), l
	ld	-5355(ix), h
	ld	l, -5356(ix)
	ld	h, -5355(ix)
	dec	sp
	dec	sp
	ld	-5358(ix), l
	ld	-5357(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5360(ix), l
	ld	-5359(ix), h
	ld	l, -5358(ix)
	ld	h, -5357(ix)
	push	hl
	ld	l, -5360(ix)
	ld	h, -5359(ix)
	ld	b, l
	pop	hl
__shift_8098:
	ld	a, b
	or	a, a
	jp	z, __sdone_4425
	add	hl, hl
	djnz	__shift_8098
__sdone_4425:
	dec	sp
	dec	sp
	ld	-5362(ix), l
	ld	-5361(ix), h
	ld	l, -5362(ix)
	ld	h, -5361(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71472
	ld	hl, #0
	jp	__cmp_e_4978
__cmp_t_71472:
	ld	hl, #1
__cmp_e_4978:
	dec	sp
	dec	sp
	ld	-5364(ix), l
	ld	-5363(ix), h
	ld	l, -5364(ix)
	ld	h, -5363(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_82853
	ld	hl, #0
	jp	__cmp_e_86966
__cmp_t_82853:
	ld	hl, #1
__cmp_e_86966:
	dec	sp
	dec	sp
	ld	-5366(ix), l
	ld	-5365(ix), h
	ld	l, -5366(ix)
	ld	h, -5365(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1144
	jp	__xcc_L1145
__xcc_L1145:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5368(ix), l
	ld	-5367(ix), h
	ld	l, -5368(ix)
	ld	h, -5367(ix)
	dec	sp
	dec	sp
	ld	-5370(ix), l
	ld	-5369(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5372(ix), l
	ld	-5371(ix), h
	ld	l, -5370(ix)
	ld	h, -5369(ix)
	push	hl
	ld	l, -5372(ix)
	ld	h, -5371(ix)
	ld	b, l
	pop	hl
__shift_4797:
	ld	a, b
	or	a, a
	jp	z, __sdone_6748
	add	hl, hl
	djnz	__shift_4797
__sdone_6748:
	dec	sp
	dec	sp
	ld	-5374(ix), l
	ld	-5373(ix), h
	ld	l, -5374(ix)
	ld	h, -5373(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5376(ix), l
	ld	-5375(ix), h
	ld	l, -5376(ix)
	ld	h, -5375(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_24547
	ld	hl, #0
	jp	__cmp_e_22272
__cmp_t_24547:
	ld	hl, #1
__cmp_e_22272:
	dec	sp
	dec	sp
	ld	-5378(ix), l
	ld	-5377(ix), h
	ld	l, -5378(ix)
	ld	h, -5377(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_40516
	ld	hl, #0
	jp	__cmp_e_81086
__cmp_t_40516:
	ld	hl, #1
__cmp_e_81086:
	dec	sp
	dec	sp
	ld	-5380(ix), l
	ld	-5379(ix), h
	jp	__xcc_L1146
__xcc_L1144:
	ld	hl, #1
	ld	-5380(ix), l
	ld	-5379(ix), h
__xcc_L1146:
	ld	l, -5380(ix)
	ld	h, -5379(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1141
	jp	__xcc_L1142
__xcc_L1141:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5382(ix), l
	ld	-5381(ix), h
	ld	l, -5382(ix)
	ld	h, -5381(ix)
	dec	sp
	dec	sp
	ld	-5384(ix), l
	ld	-5383(ix), h
	jp	__xcc_L1143
__xcc_L1142:
	ld	hl, #1
	ld	-5384(ix), l
	ld	-5383(ix), h
__xcc_L1143:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5386(ix), l
	ld	-5385(ix), h
	.globl __mul16
	ld	l, -5386(ix)
	ld	h, -5385(ix)
	push	hl
	ld	l, -5384(ix)
	ld	h, -5383(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5388(ix), l
	ld	-5387(ix), h
	ld	l, -5388(ix)
	ld	h, -5387(ix)
	push	hl
	ld	l, -5354(ix)
	ld	h, -5353(ix)
	push	hl
	ld	l, -5328(ix)
	ld	h, -5327(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1147
	dec	sp
	dec	sp
	ld	-5390(ix), l
	ld	-5389(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5392(ix), l
	ld	-5391(ix), h
	ld	l, -5392(ix)
	ld	h, -5391(ix)
	dec	sp
	dec	sp
	ld	-5394(ix), l
	ld	-5393(ix), h
	ld	l, -5394(ix)
	ld	h, -5393(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_9912
	ld	hl, #0
	jp	__cmp_e_65159
__cmp_t_9912:
	ld	hl, #1
__cmp_e_65159:
	dec	sp
	dec	sp
	ld	-5396(ix), l
	ld	-5395(ix), h
	ld	l, -5396(ix)
	ld	h, -5395(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89877
	ld	hl, #0
	jp	__cmp_e_61900
__cmp_t_89877:
	ld	hl, #1
__cmp_e_61900:
	dec	sp
	dec	sp
	ld	-5398(ix), l
	ld	-5397(ix), h
	ld	l, -5398(ix)
	ld	h, -5397(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1151
	jp	__xcc_L1152
__xcc_L1152:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5400(ix), l
	ld	-5399(ix), h
	ld	l, -5400(ix)
	ld	h, -5399(ix)
	dec	sp
	dec	sp
	ld	-5402(ix), l
	ld	-5401(ix), h
	ld	l, -5402(ix)
	ld	h, -5401(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5404(ix), l
	ld	-5403(ix), h
	ld	l, -5404(ix)
	ld	h, -5403(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72553
	ld	hl, #0
	jp	__cmp_e_41197
__cmp_t_72553:
	ld	hl, #1
__cmp_e_41197:
	dec	sp
	dec	sp
	ld	-5406(ix), l
	ld	-5405(ix), h
	ld	l, -5406(ix)
	ld	h, -5405(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_62932
	ld	hl, #0
	jp	__cmp_e_43003
__cmp_t_62932:
	ld	hl, #1
__cmp_e_43003:
	dec	sp
	dec	sp
	ld	-5408(ix), l
	ld	-5407(ix), h
	jp	__xcc_L1153
__xcc_L1151:
	ld	hl, #1
	ld	-5408(ix), l
	ld	-5407(ix), h
__xcc_L1153:
	ld	l, -5408(ix)
	ld	h, -5407(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1148
	jp	__xcc_L1149
__xcc_L1148:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5410(ix), l
	ld	-5409(ix), h
	ld	l, -5410(ix)
	ld	h, -5409(ix)
	dec	sp
	dec	sp
	ld	-5412(ix), l
	ld	-5411(ix), h
	jp	__xcc_L1150
__xcc_L1149:
	ld	hl, #1
	ld	-5412(ix), l
	ld	-5411(ix), h
__xcc_L1150:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5414(ix), l
	ld	-5413(ix), h
	.globl __mul16
	ld	l, -5414(ix)
	ld	h, -5413(ix)
	push	hl
	ld	l, -5412(ix)
	ld	h, -5411(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5416(ix), l
	ld	-5415(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5418(ix), l
	ld	-5417(ix), h
	ld	l, -5418(ix)
	ld	h, -5417(ix)
	dec	sp
	dec	sp
	ld	-5420(ix), l
	ld	-5419(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5422(ix), l
	ld	-5421(ix), h
	ld	l, -5420(ix)
	ld	h, -5419(ix)
	push	hl
	ld	l, -5422(ix)
	ld	h, -5421(ix)
	ld	b, l
	pop	hl
__shift_2035:
	ld	a, b
	or	a, a
	jp	z, __sdone_5951
	add	hl, hl
	djnz	__shift_2035
__sdone_5951:
	dec	sp
	dec	sp
	ld	-5424(ix), l
	ld	-5423(ix), h
	ld	l, -5424(ix)
	ld	h, -5423(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18107
	ld	hl, #0
	jp	__cmp_e_48498
__cmp_t_18107:
	ld	hl, #1
__cmp_e_48498:
	dec	sp
	dec	sp
	ld	-5426(ix), l
	ld	-5425(ix), h
	ld	l, -5426(ix)
	ld	h, -5425(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10049
	ld	hl, #0
	jp	__cmp_e_29154
__cmp_t_10049:
	ld	hl, #1
__cmp_e_29154:
	dec	sp
	dec	sp
	ld	-5428(ix), l
	ld	-5427(ix), h
	ld	l, -5428(ix)
	ld	h, -5427(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1157
	jp	__xcc_L1158
__xcc_L1158:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5430(ix), l
	ld	-5429(ix), h
	ld	l, -5430(ix)
	ld	h, -5429(ix)
	dec	sp
	dec	sp
	ld	-5432(ix), l
	ld	-5431(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5434(ix), l
	ld	-5433(ix), h
	ld	l, -5432(ix)
	ld	h, -5431(ix)
	push	hl
	ld	l, -5434(ix)
	ld	h, -5433(ix)
	ld	b, l
	pop	hl
__shift_3861:
	ld	a, b
	or	a, a
	jp	z, __sdone_2906
	add	hl, hl
	djnz	__shift_3861
__sdone_2906:
	dec	sp
	dec	sp
	ld	-5436(ix), l
	ld	-5435(ix), h
	ld	l, -5436(ix)
	ld	h, -5435(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5438(ix), l
	ld	-5437(ix), h
	ld	l, -5438(ix)
	ld	h, -5437(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92334
	ld	hl, #0
	jp	__cmp_e_70003
__cmp_t_92334:
	ld	hl, #1
__cmp_e_70003:
	dec	sp
	dec	sp
	ld	-5440(ix), l
	ld	-5439(ix), h
	ld	l, -5440(ix)
	ld	h, -5439(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_35325
	ld	hl, #0
	jp	__cmp_e_76784
__cmp_t_35325:
	ld	hl, #1
__cmp_e_76784:
	dec	sp
	dec	sp
	ld	-5442(ix), l
	ld	-5441(ix), h
	jp	__xcc_L1159
__xcc_L1157:
	ld	hl, #1
	ld	-5442(ix), l
	ld	-5441(ix), h
__xcc_L1159:
	ld	l, -5442(ix)
	ld	h, -5441(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1154
	jp	__xcc_L1155
__xcc_L1154:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5444(ix), l
	ld	-5443(ix), h
	ld	l, -5444(ix)
	ld	h, -5443(ix)
	dec	sp
	dec	sp
	ld	-5446(ix), l
	ld	-5445(ix), h
	jp	__xcc_L1156
__xcc_L1155:
	ld	hl, #1
	ld	-5446(ix), l
	ld	-5445(ix), h
__xcc_L1156:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5448(ix), l
	ld	-5447(ix), h
	.globl __mul16
	ld	l, -5448(ix)
	ld	h, -5447(ix)
	push	hl
	ld	l, -5446(ix)
	ld	h, -5445(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5450(ix), l
	ld	-5449(ix), h
	ld	l, -5450(ix)
	ld	h, -5449(ix)
	push	hl
	ld	l, -5416(ix)
	ld	h, -5415(ix)
	push	hl
	ld	l, -5390(ix)
	ld	h, -5389(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1132:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1131
	jp	__xcc_L1133
__xcc_L1133:
__xcc_L1160:
	ld	hl, #__str_1163
	dec	sp
	dec	sp
	ld	-5452(ix), l
	ld	-5451(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5454(ix), l
	ld	-5453(ix), h
	ld	l, -5454(ix)
	ld	h, -5453(ix)
	dec	sp
	dec	sp
	ld	-5456(ix), l
	ld	-5455(ix), h
	ld	l, -5456(ix)
	ld	h, -5455(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_64428
	ld	hl, #0
	jp	__cmp_e_6797
__cmp_t_64428:
	ld	hl, #1
__cmp_e_6797:
	dec	sp
	dec	sp
	ld	-5458(ix), l
	ld	-5457(ix), h
	ld	l, -5458(ix)
	ld	h, -5457(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81763
	ld	hl, #0
	jp	__cmp_e_63633
__cmp_t_81763:
	ld	hl, #1
__cmp_e_63633:
	dec	sp
	dec	sp
	ld	-5460(ix), l
	ld	-5459(ix), h
	ld	l, -5460(ix)
	ld	h, -5459(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1167
	jp	__xcc_L1168
__xcc_L1168:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5462(ix), l
	ld	-5461(ix), h
	ld	l, -5462(ix)
	ld	h, -5461(ix)
	dec	sp
	dec	sp
	ld	-5464(ix), l
	ld	-5463(ix), h
	ld	l, -5464(ix)
	ld	h, -5463(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5466(ix), l
	ld	-5465(ix), h
	ld	l, -5466(ix)
	ld	h, -5465(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_10115
	ld	hl, #0
	jp	__cmp_e_46560
__cmp_t_10115:
	ld	hl, #1
__cmp_e_46560:
	dec	sp
	dec	sp
	ld	-5468(ix), l
	ld	-5467(ix), h
	ld	l, -5468(ix)
	ld	h, -5467(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80381
	ld	hl, #0
	jp	__cmp_e_51014
__cmp_t_80381:
	ld	hl, #1
__cmp_e_51014:
	dec	sp
	dec	sp
	ld	-5470(ix), l
	ld	-5469(ix), h
	jp	__xcc_L1169
__xcc_L1167:
	ld	hl, #1
	ld	-5470(ix), l
	ld	-5469(ix), h
__xcc_L1169:
	ld	l, -5470(ix)
	ld	h, -5469(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1164
	jp	__xcc_L1165
__xcc_L1164:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5472(ix), l
	ld	-5471(ix), h
	ld	l, -5472(ix)
	ld	h, -5471(ix)
	dec	sp
	dec	sp
	ld	-5474(ix), l
	ld	-5473(ix), h
	jp	__xcc_L1166
__xcc_L1165:
	ld	hl, #1
	ld	-5474(ix), l
	ld	-5473(ix), h
__xcc_L1166:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5476(ix), l
	ld	-5475(ix), h
	.globl __mul16
	ld	l, -5476(ix)
	ld	h, -5475(ix)
	push	hl
	ld	l, -5474(ix)
	ld	h, -5473(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5478(ix), l
	ld	-5477(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5480(ix), l
	ld	-5479(ix), h
	ld	l, -5480(ix)
	ld	h, -5479(ix)
	dec	sp
	dec	sp
	ld	-5482(ix), l
	ld	-5481(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5486(ix), l
	ld	-5485(ix), h
	ld	l, -5482(ix)
	ld	h, -5481(ix)
	push	hl
	ld	l, -5486(ix)
	ld	h, -5485(ix)
	ld	b, l
	pop	hl
__shift_5185:
	ld	a, b
	or	a, a
	jp	z, __sdone_897
	add	hl, hl
	djnz	__shift_5185
__sdone_897:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5490(ix), l
	ld	-5489(ix), h
	ld	l, -5490(ix)
	ld	h, -5489(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_32100
	ld	hl, #0
	jp	__cmp_e_95097
__cmp_t_32100:
	ld	hl, #1
__cmp_e_95097:
	dec	sp
	dec	sp
	ld	-5492(ix), l
	ld	-5491(ix), h
	ld	l, -5492(ix)
	ld	h, -5491(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2408
	ld	hl, #0
	jp	__cmp_e_38329
__cmp_t_2408:
	ld	hl, #1
__cmp_e_38329:
	dec	sp
	dec	sp
	ld	-5494(ix), l
	ld	-5493(ix), h
	ld	l, -5494(ix)
	ld	h, -5493(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1173
	jp	__xcc_L1174
__xcc_L1174:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5496(ix), l
	ld	-5495(ix), h
	ld	l, -5496(ix)
	ld	h, -5495(ix)
	dec	sp
	dec	sp
	ld	-5498(ix), l
	ld	-5497(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5502(ix), l
	ld	-5501(ix), h
	ld	l, -5498(ix)
	ld	h, -5497(ix)
	push	hl
	ld	l, -5502(ix)
	ld	h, -5501(ix)
	ld	b, l
	pop	hl
__shift_3349:
	ld	a, b
	or	a, a
	jp	z, __sdone_1313
	add	hl, hl
	djnz	__shift_3349
__sdone_1313:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5506(ix), l
	ld	-5505(ix), h
	ld	l, -5506(ix)
	ld	h, -5505(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5508(ix), l
	ld	-5507(ix), h
	ld	l, -5508(ix)
	ld	h, -5507(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_95879
	ld	hl, #0
	jp	__cmp_e_52634
__cmp_t_95879:
	ld	hl, #1
__cmp_e_52634:
	dec	sp
	dec	sp
	ld	-5510(ix), l
	ld	-5509(ix), h
	ld	l, -5510(ix)
	ld	h, -5509(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34316
	ld	hl, #0
	jp	__cmp_e_57914
__cmp_t_34316:
	ld	hl, #1
__cmp_e_57914:
	dec	sp
	dec	sp
	ld	-5512(ix), l
	ld	-5511(ix), h
	jp	__xcc_L1175
__xcc_L1173:
	ld	hl, #1
	ld	-5512(ix), l
	ld	-5511(ix), h
__xcc_L1175:
	ld	l, -5512(ix)
	ld	h, -5511(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1170
	jp	__xcc_L1171
__xcc_L1170:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5514(ix), l
	ld	-5513(ix), h
	ld	l, -5514(ix)
	ld	h, -5513(ix)
	dec	sp
	dec	sp
	ld	-5516(ix), l
	ld	-5515(ix), h
	jp	__xcc_L1172
__xcc_L1171:
	ld	hl, #1
	ld	-5516(ix), l
	ld	-5515(ix), h
__xcc_L1172:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-5518(ix), l
	ld	-5517(ix), h
	.globl __mul16
	ld	l, -5518(ix)
	ld	h, -5517(ix)
	push	hl
	ld	l, -5516(ix)
	ld	h, -5515(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5520(ix), l
	ld	-5519(ix), h
	ld	l, -5520(ix)
	ld	h, -5519(ix)
	push	hl
	ld	l, -5478(ix)
	ld	h, -5477(ix)
	push	hl
	ld	l, -5452(ix)
	ld	h, -5451(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1176
	dec	sp
	dec	sp
	ld	-5522(ix), l
	ld	-5521(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5524(ix), l
	ld	-5523(ix), h
	ld	l, -5524(ix)
	ld	h, -5523(ix)
	dec	sp
	dec	sp
	ld	-5526(ix), l
	ld	-5525(ix), h
	ld	l, -5526(ix)
	ld	h, -5525(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_8585
	ld	hl, #0
	jp	__cmp_e_68775
__cmp_t_8585:
	ld	hl, #1
__cmp_e_68775:
	dec	sp
	dec	sp
	ld	-5528(ix), l
	ld	-5527(ix), h
	ld	l, -5528(ix)
	ld	h, -5527(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22765
	ld	hl, #0
	jp	__cmp_e_34986
__cmp_t_22765:
	ld	hl, #1
__cmp_e_34986:
	dec	sp
	dec	sp
	ld	-5530(ix), l
	ld	-5529(ix), h
	ld	l, -5530(ix)
	ld	h, -5529(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1180
	jp	__xcc_L1181
__xcc_L1181:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5532(ix), l
	ld	-5531(ix), h
	ld	l, -5532(ix)
	ld	h, -5531(ix)
	dec	sp
	dec	sp
	ld	-5534(ix), l
	ld	-5533(ix), h
	ld	l, -5534(ix)
	ld	h, -5533(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5536(ix), l
	ld	-5535(ix), h
	ld	l, -5536(ix)
	ld	h, -5535(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97930
	ld	hl, #0
	jp	__cmp_e_36626
__cmp_t_97930:
	ld	hl, #1
__cmp_e_36626:
	dec	sp
	dec	sp
	ld	-5538(ix), l
	ld	-5537(ix), h
	ld	l, -5538(ix)
	ld	h, -5537(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_37892
	ld	hl, #0
	jp	__cmp_e_6616
__cmp_t_37892:
	ld	hl, #1
__cmp_e_6616:
	dec	sp
	dec	sp
	ld	-5540(ix), l
	ld	-5539(ix), h
	jp	__xcc_L1182
__xcc_L1180:
	ld	hl, #1
	ld	-5540(ix), l
	ld	-5539(ix), h
__xcc_L1182:
	ld	l, -5540(ix)
	ld	h, -5539(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1177
	jp	__xcc_L1178
__xcc_L1177:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5542(ix), l
	ld	-5541(ix), h
	ld	l, -5542(ix)
	ld	h, -5541(ix)
	dec	sp
	dec	sp
	ld	-5544(ix), l
	ld	-5543(ix), h
	jp	__xcc_L1179
__xcc_L1178:
	ld	hl, #1
	ld	-5544(ix), l
	ld	-5543(ix), h
__xcc_L1179:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5546(ix), l
	ld	-5545(ix), h
	.globl __mul16
	ld	l, -5546(ix)
	ld	h, -5545(ix)
	push	hl
	ld	l, -5544(ix)
	ld	h, -5543(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5548(ix), l
	ld	-5547(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5550(ix), l
	ld	-5549(ix), h
	ld	l, -5550(ix)
	ld	h, -5549(ix)
	dec	sp
	dec	sp
	ld	-5552(ix), l
	ld	-5551(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5556(ix), l
	ld	-5555(ix), h
	ld	l, -5552(ix)
	ld	h, -5551(ix)
	push	hl
	ld	l, -5556(ix)
	ld	h, -5555(ix)
	ld	b, l
	pop	hl
__shift_6629:
	ld	a, b
	or	a, a
	jp	z, __sdone_9569
	add	hl, hl
	djnz	__shift_6629
__sdone_9569:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5560(ix), l
	ld	-5559(ix), h
	ld	l, -5560(ix)
	ld	h, -5559(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99752
	ld	hl, #0
	jp	__cmp_e_87409
__cmp_t_99752:
	ld	hl, #1
__cmp_e_87409:
	dec	sp
	dec	sp
	ld	-5562(ix), l
	ld	-5561(ix), h
	ld	l, -5562(ix)
	ld	h, -5561(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96366
	ld	hl, #0
	jp	__cmp_e_81515
__cmp_t_96366:
	ld	hl, #1
__cmp_e_81515:
	dec	sp
	dec	sp
	ld	-5564(ix), l
	ld	-5563(ix), h
	ld	l, -5564(ix)
	ld	h, -5563(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1186
	jp	__xcc_L1187
__xcc_L1187:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5566(ix), l
	ld	-5565(ix), h
	ld	l, -5566(ix)
	ld	h, -5565(ix)
	dec	sp
	dec	sp
	ld	-5568(ix), l
	ld	-5567(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5572(ix), l
	ld	-5571(ix), h
	ld	l, -5568(ix)
	ld	h, -5567(ix)
	push	hl
	ld	l, -5572(ix)
	ld	h, -5571(ix)
	ld	b, l
	pop	hl
__shift_7395:
	ld	a, b
	or	a, a
	jp	z, __sdone_2833
	add	hl, hl
	djnz	__shift_7395
__sdone_2833:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5576(ix), l
	ld	-5575(ix), h
	ld	l, -5576(ix)
	ld	h, -5575(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5578(ix), l
	ld	-5577(ix), h
	ld	l, -5578(ix)
	ld	h, -5577(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_44428
	ld	hl, #0
	jp	__cmp_e_47776
__cmp_t_44428:
	ld	hl, #1
__cmp_e_47776:
	dec	sp
	dec	sp
	ld	-5580(ix), l
	ld	-5579(ix), h
	ld	l, -5580(ix)
	ld	h, -5579(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73847
	ld	hl, #0
	jp	__cmp_e_29613
__cmp_t_73847:
	ld	hl, #1
__cmp_e_29613:
	dec	sp
	dec	sp
	ld	-5582(ix), l
	ld	-5581(ix), h
	jp	__xcc_L1188
__xcc_L1186:
	ld	hl, #1
	ld	-5582(ix), l
	ld	-5581(ix), h
__xcc_L1188:
	ld	l, -5582(ix)
	ld	h, -5581(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1183
	jp	__xcc_L1184
__xcc_L1183:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5584(ix), l
	ld	-5583(ix), h
	ld	l, -5584(ix)
	ld	h, -5583(ix)
	dec	sp
	dec	sp
	ld	-5586(ix), l
	ld	-5585(ix), h
	jp	__xcc_L1185
__xcc_L1184:
	ld	hl, #1
	ld	-5586(ix), l
	ld	-5585(ix), h
__xcc_L1185:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-5588(ix), l
	ld	-5587(ix), h
	.globl __mul16
	ld	l, -5588(ix)
	ld	h, -5587(ix)
	push	hl
	ld	l, -5586(ix)
	ld	h, -5585(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5590(ix), l
	ld	-5589(ix), h
	ld	l, -5590(ix)
	ld	h, -5589(ix)
	push	hl
	ld	l, -5548(ix)
	ld	h, -5547(ix)
	push	hl
	ld	l, -5522(ix)
	ld	h, -5521(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1161:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1160
	jp	__xcc_L1162
__xcc_L1162:
__xcc_L1189:
	ld	hl, #__str_1192
	dec	sp
	dec	sp
	ld	-5592(ix), l
	ld	-5591(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5594(ix), l
	ld	-5593(ix), h
	ld	l, -5594(ix)
	ld	h, -5593(ix)
	dec	sp
	dec	sp
	ld	-5596(ix), l
	ld	-5595(ix), h
	ld	l, -5596(ix)
	ld	h, -5595(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85026
	ld	hl, #0
	jp	__cmp_e_22300
__cmp_t_85026:
	ld	hl, #1
__cmp_e_22300:
	dec	sp
	dec	sp
	ld	-5598(ix), l
	ld	-5597(ix), h
	ld	l, -5598(ix)
	ld	h, -5597(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41062
	ld	hl, #0
	jp	__cmp_e_3786
__cmp_t_41062:
	ld	hl, #1
__cmp_e_3786:
	dec	sp
	dec	sp
	ld	-5600(ix), l
	ld	-5599(ix), h
	ld	l, -5600(ix)
	ld	h, -5599(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1196
	jp	__xcc_L1197
__xcc_L1197:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5602(ix), l
	ld	-5601(ix), h
	ld	l, -5602(ix)
	ld	h, -5601(ix)
	dec	sp
	dec	sp
	ld	-5604(ix), l
	ld	-5603(ix), h
	ld	l, -5604(ix)
	ld	h, -5603(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5606(ix), l
	ld	-5605(ix), h
	ld	l, -5606(ix)
	ld	h, -5605(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60629
	ld	hl, #0
	jp	__cmp_e_30763
__cmp_t_60629:
	ld	hl, #1
__cmp_e_30763:
	dec	sp
	dec	sp
	ld	-5608(ix), l
	ld	-5607(ix), h
	ld	l, -5608(ix)
	ld	h, -5607(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95100
	ld	hl, #0
	jp	__cmp_e_56508
__cmp_t_95100:
	ld	hl, #1
__cmp_e_56508:
	dec	sp
	dec	sp
	ld	-5610(ix), l
	ld	-5609(ix), h
	jp	__xcc_L1198
__xcc_L1196:
	ld	hl, #1
	ld	-5610(ix), l
	ld	-5609(ix), h
__xcc_L1198:
	ld	l, -5610(ix)
	ld	h, -5609(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1193
	jp	__xcc_L1194
__xcc_L1193:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5612(ix), l
	ld	-5611(ix), h
	ld	l, -5612(ix)
	ld	h, -5611(ix)
	dec	sp
	dec	sp
	ld	-5614(ix), l
	ld	-5613(ix), h
	jp	__xcc_L1195
__xcc_L1194:
	ld	hl, #1
	ld	-5614(ix), l
	ld	-5613(ix), h
__xcc_L1195:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5616(ix), l
	ld	-5615(ix), h
	.globl __mul16
	ld	l, -5616(ix)
	ld	h, -5615(ix)
	push	hl
	ld	l, -5614(ix)
	ld	h, -5613(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5618(ix), l
	ld	-5617(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5620(ix), l
	ld	-5619(ix), h
	ld	l, -5620(ix)
	ld	h, -5619(ix)
	dec	sp
	dec	sp
	ld	-5622(ix), l
	ld	-5621(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5630(ix), l
	ld	-5629(ix), h
	ld	l, -5622(ix)
	ld	h, -5621(ix)
	push	hl
	ld	l, -5630(ix)
	ld	h, -5629(ix)
	ld	b, l
	pop	hl
__shift_3397:
	ld	a, b
	or	a, a
	jp	z, __sdone_9416
	add	hl, hl
	djnz	__shift_3397
__sdone_9416:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5638(ix), l
	ld	-5637(ix), h
	ld	l, -5638(ix)
	ld	h, -5637(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30775
	ld	hl, #0
	jp	__cmp_e_91982
__cmp_t_30775:
	ld	hl, #1
__cmp_e_91982:
	dec	sp
	dec	sp
	ld	-5640(ix), l
	ld	-5639(ix), h
	ld	l, -5640(ix)
	ld	h, -5639(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14544
	ld	hl, #0
	jp	__cmp_e_53540
__cmp_t_14544:
	ld	hl, #1
__cmp_e_53540:
	dec	sp
	dec	sp
	ld	-5642(ix), l
	ld	-5641(ix), h
	ld	l, -5642(ix)
	ld	h, -5641(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1202
	jp	__xcc_L1203
__xcc_L1203:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5644(ix), l
	ld	-5643(ix), h
	ld	l, -5644(ix)
	ld	h, -5643(ix)
	dec	sp
	dec	sp
	ld	-5646(ix), l
	ld	-5645(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5654(ix), l
	ld	-5653(ix), h
	ld	l, -5646(ix)
	ld	h, -5645(ix)
	push	hl
	ld	l, -5654(ix)
	ld	h, -5653(ix)
	ld	b, l
	pop	hl
__shift_3320:
	ld	a, b
	or	a, a
	jp	z, __sdone_8826
	add	hl, hl
	djnz	__shift_3320
__sdone_8826:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5662(ix), l
	ld	-5661(ix), h
	ld	l, -5662(ix)
	ld	h, -5661(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5664(ix), l
	ld	-5663(ix), h
	ld	l, -5664(ix)
	ld	h, -5663(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_6518
	ld	hl, #0
	jp	__cmp_e_97565
__cmp_t_6518:
	ld	hl, #1
__cmp_e_97565:
	dec	sp
	dec	sp
	ld	-5666(ix), l
	ld	-5665(ix), h
	ld	l, -5666(ix)
	ld	h, -5665(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_51794
	ld	hl, #0
	jp	__cmp_e_29499
__cmp_t_51794:
	ld	hl, #1
__cmp_e_29499:
	dec	sp
	dec	sp
	ld	-5668(ix), l
	ld	-5667(ix), h
	jp	__xcc_L1204
__xcc_L1202:
	ld	hl, #1
	ld	-5668(ix), l
	ld	-5667(ix), h
__xcc_L1204:
	ld	l, -5668(ix)
	ld	h, -5667(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1199
	jp	__xcc_L1200
__xcc_L1199:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5670(ix), l
	ld	-5669(ix), h
	ld	l, -5670(ix)
	ld	h, -5669(ix)
	dec	sp
	dec	sp
	ld	-5672(ix), l
	ld	-5671(ix), h
	jp	__xcc_L1201
__xcc_L1200:
	ld	hl, #1
	ld	-5672(ix), l
	ld	-5671(ix), h
__xcc_L1201:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-5674(ix), l
	ld	-5673(ix), h
	.globl __mul16
	ld	l, -5674(ix)
	ld	h, -5673(ix)
	push	hl
	ld	l, -5672(ix)
	ld	h, -5671(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5676(ix), l
	ld	-5675(ix), h
	ld	l, -5676(ix)
	ld	h, -5675(ix)
	push	hl
	ld	l, -5618(ix)
	ld	h, -5617(ix)
	push	hl
	ld	l, -5592(ix)
	ld	h, -5591(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1205
	dec	sp
	dec	sp
	ld	-5678(ix), l
	ld	-5677(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5680(ix), l
	ld	-5679(ix), h
	ld	l, -5680(ix)
	ld	h, -5679(ix)
	dec	sp
	dec	sp
	ld	-5682(ix), l
	ld	-5681(ix), h
	ld	l, -5682(ix)
	ld	h, -5681(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87134
	ld	hl, #0
	jp	__cmp_e_51546
__cmp_t_87134:
	ld	hl, #1
__cmp_e_51546:
	dec	sp
	dec	sp
	ld	-5684(ix), l
	ld	-5683(ix), h
	ld	l, -5684(ix)
	ld	h, -5683(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16908
	ld	hl, #0
	jp	__cmp_e_99853
__cmp_t_16908:
	ld	hl, #1
__cmp_e_99853:
	dec	sp
	dec	sp
	ld	-5686(ix), l
	ld	-5685(ix), h
	ld	l, -5686(ix)
	ld	h, -5685(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1209
	jp	__xcc_L1210
__xcc_L1210:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5688(ix), l
	ld	-5687(ix), h
	ld	l, -5688(ix)
	ld	h, -5687(ix)
	dec	sp
	dec	sp
	ld	-5690(ix), l
	ld	-5689(ix), h
	ld	l, -5690(ix)
	ld	h, -5689(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5692(ix), l
	ld	-5691(ix), h
	ld	l, -5692(ix)
	ld	h, -5691(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_33062
	ld	hl, #0
	jp	__cmp_e_84303
__cmp_t_33062:
	ld	hl, #1
__cmp_e_84303:
	dec	sp
	dec	sp
	ld	-5694(ix), l
	ld	-5693(ix), h
	ld	l, -5694(ix)
	ld	h, -5693(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22686
	ld	hl, #0
	jp	__cmp_e_93842
__cmp_t_22686:
	ld	hl, #1
__cmp_e_93842:
	dec	sp
	dec	sp
	ld	-5696(ix), l
	ld	-5695(ix), h
	jp	__xcc_L1211
__xcc_L1209:
	ld	hl, #1
	ld	-5696(ix), l
	ld	-5695(ix), h
__xcc_L1211:
	ld	l, -5696(ix)
	ld	h, -5695(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1206
	jp	__xcc_L1207
__xcc_L1206:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5698(ix), l
	ld	-5697(ix), h
	ld	l, -5698(ix)
	ld	h, -5697(ix)
	dec	sp
	dec	sp
	ld	-5700(ix), l
	ld	-5699(ix), h
	jp	__xcc_L1208
__xcc_L1207:
	ld	hl, #1
	ld	-5700(ix), l
	ld	-5699(ix), h
__xcc_L1208:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5702(ix), l
	ld	-5701(ix), h
	.globl __mul16
	ld	l, -5702(ix)
	ld	h, -5701(ix)
	push	hl
	ld	l, -5700(ix)
	ld	h, -5699(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5704(ix), l
	ld	-5703(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5706(ix), l
	ld	-5705(ix), h
	ld	l, -5706(ix)
	ld	h, -5705(ix)
	dec	sp
	dec	sp
	ld	-5708(ix), l
	ld	-5707(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5716(ix), l
	ld	-5715(ix), h
	ld	l, -5708(ix)
	ld	h, -5707(ix)
	push	hl
	ld	l, -5716(ix)
	ld	h, -5715(ix)
	ld	b, l
	pop	hl
__shift_8432:
	ld	a, b
	or	a, a
	jp	z, __sdone_6534
	add	hl, hl
	djnz	__shift_8432
__sdone_6534:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5724(ix), l
	ld	-5723(ix), h
	ld	l, -5724(ix)
	ld	h, -5723(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39807
	ld	hl, #0
	jp	__cmp_e_49810
__cmp_t_39807:
	ld	hl, #1
__cmp_e_49810:
	dec	sp
	dec	sp
	ld	-5726(ix), l
	ld	-5725(ix), h
	ld	l, -5726(ix)
	ld	h, -5725(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_18834
	ld	hl, #0
	jp	__cmp_e_80869
__cmp_t_18834:
	ld	hl, #1
__cmp_e_80869:
	dec	sp
	dec	sp
	ld	-5728(ix), l
	ld	-5727(ix), h
	ld	l, -5728(ix)
	ld	h, -5727(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1215
	jp	__xcc_L1216
__xcc_L1216:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5730(ix), l
	ld	-5729(ix), h
	ld	l, -5730(ix)
	ld	h, -5729(ix)
	dec	sp
	dec	sp
	ld	-5732(ix), l
	ld	-5731(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5740(ix), l
	ld	-5739(ix), h
	ld	l, -5732(ix)
	ld	h, -5731(ix)
	push	hl
	ld	l, -5740(ix)
	ld	h, -5739(ix)
	ld	b, l
	pop	hl
__shift_3596:
	ld	a, b
	or	a, a
	jp	z, __sdone_5815
	add	hl, hl
	djnz	__shift_3596
__sdone_5815:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-5748(ix), l
	ld	-5747(ix), h
	ld	l, -5748(ix)
	ld	h, -5747(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5750(ix), l
	ld	-5749(ix), h
	ld	l, -5750(ix)
	ld	h, -5749(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_27984
	ld	hl, #0
	jp	__cmp_e_48696
__cmp_t_27984:
	ld	hl, #1
__cmp_e_48696:
	dec	sp
	dec	sp
	ld	-5752(ix), l
	ld	-5751(ix), h
	ld	l, -5752(ix)
	ld	h, -5751(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52324
	ld	hl, #0
	jp	__cmp_e_11382
__cmp_t_52324:
	ld	hl, #1
__cmp_e_11382:
	dec	sp
	dec	sp
	ld	-5754(ix), l
	ld	-5753(ix), h
	jp	__xcc_L1217
__xcc_L1215:
	ld	hl, #1
	ld	-5754(ix), l
	ld	-5753(ix), h
__xcc_L1217:
	ld	l, -5754(ix)
	ld	h, -5753(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1212
	jp	__xcc_L1213
__xcc_L1212:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5756(ix), l
	ld	-5755(ix), h
	ld	l, -5756(ix)
	ld	h, -5755(ix)
	dec	sp
	dec	sp
	ld	-5758(ix), l
	ld	-5757(ix), h
	jp	__xcc_L1214
__xcc_L1213:
	ld	hl, #1
	ld	-5758(ix), l
	ld	-5757(ix), h
__xcc_L1214:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-5760(ix), l
	ld	-5759(ix), h
	.globl __mul16
	ld	l, -5760(ix)
	ld	h, -5759(ix)
	push	hl
	ld	l, -5758(ix)
	ld	h, -5757(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5762(ix), l
	ld	-5761(ix), h
	ld	l, -5762(ix)
	ld	h, -5761(ix)
	push	hl
	ld	l, -5704(ix)
	ld	h, -5703(ix)
	push	hl
	ld	l, -5678(ix)
	ld	h, -5677(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1190:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1189
	jp	__xcc_L1191
__xcc_L1191:
__xcc_L1100:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1099
	jp	__xcc_L1101
__xcc_L1101:
__xcc_L978:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L977
	jp	__xcc_L979
__xcc_L979:
__xcc_L1218:
__xcc_L1221:
__xcc_L1224:
	ld	hl, #__str_1227
	dec	sp
	dec	sp
	ld	-5764(ix), l
	ld	-5763(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5766(ix), l
	ld	-5765(ix), h
	ld	l, -5766(ix)
	ld	h, -5765(ix)
	dec	sp
	dec	sp
	ld	-5768(ix), l
	ld	-5767(ix), h
	ld	l, -5768(ix)
	ld	h, -5767(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_94465
	ld	hl, #0
	jp	__cmp_e_99451
__cmp_t_94465:
	ld	hl, #1
__cmp_e_99451:
	dec	sp
	dec	sp
	ld	-5770(ix), l
	ld	-5769(ix), h
	ld	l, -5770(ix)
	ld	h, -5769(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19716
	ld	hl, #0
	jp	__cmp_e_25361
__cmp_t_19716:
	ld	hl, #1
__cmp_e_25361:
	dec	sp
	dec	sp
	ld	-5772(ix), l
	ld	-5771(ix), h
	ld	l, -5772(ix)
	ld	h, -5771(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1231
	jp	__xcc_L1232
__xcc_L1232:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5774(ix), l
	ld	-5773(ix), h
	ld	l, -5774(ix)
	ld	h, -5773(ix)
	dec	sp
	dec	sp
	ld	-5776(ix), l
	ld	-5775(ix), h
	ld	l, -5776(ix)
	ld	h, -5775(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5778(ix), l
	ld	-5777(ix), h
	ld	l, -5778(ix)
	ld	h, -5777(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_69343
	ld	hl, #0
	jp	__cmp_e_63037
__cmp_t_69343:
	ld	hl, #1
__cmp_e_63037:
	dec	sp
	dec	sp
	ld	-5780(ix), l
	ld	-5779(ix), h
	ld	l, -5780(ix)
	ld	h, -5779(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_54187
	ld	hl, #0
	jp	__cmp_e_75861
__cmp_t_54187:
	ld	hl, #1
__cmp_e_75861:
	dec	sp
	dec	sp
	ld	-5782(ix), l
	ld	-5781(ix), h
	jp	__xcc_L1233
__xcc_L1231:
	ld	hl, #1
	ld	-5782(ix), l
	ld	-5781(ix), h
__xcc_L1233:
	ld	l, -5782(ix)
	ld	h, -5781(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1228
	jp	__xcc_L1229
__xcc_L1228:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5784(ix), l
	ld	-5783(ix), h
	ld	l, -5784(ix)
	ld	h, -5783(ix)
	dec	sp
	dec	sp
	ld	-5786(ix), l
	ld	-5785(ix), h
	jp	__xcc_L1230
__xcc_L1229:
	ld	hl, #1
	ld	-5786(ix), l
	ld	-5785(ix), h
__xcc_L1230:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5788(ix), l
	ld	-5787(ix), h
	.globl __mul16
	ld	l, -5788(ix)
	ld	h, -5787(ix)
	push	hl
	ld	l, -5786(ix)
	ld	h, -5785(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5790(ix), l
	ld	-5789(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5792(ix), l
	ld	-5791(ix), h
	ld	l, -5792(ix)
	ld	h, -5791(ix)
	dec	sp
	dec	sp
	ld	-5794(ix), l
	ld	-5793(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5796(ix), l
	ld	-5795(ix), h
	ld	l, -5794(ix)
	ld	h, -5793(ix)
	push	hl
	ld	l, -5796(ix)
	ld	h, -5795(ix)
	ld	b, l
	pop	hl
__shift_602:
	ld	a, b
	or	a, a
	jp	z, __sdone_5981
	add	hl, hl
	djnz	__shift_602
__sdone_5981:
	dec	sp
	dec	sp
	ld	-5798(ix), l
	ld	-5797(ix), h
	ld	l, -5798(ix)
	ld	h, -5797(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5360
	ld	hl, #0
	jp	__cmp_e_64088
__cmp_t_5360:
	ld	hl, #1
__cmp_e_64088:
	dec	sp
	dec	sp
	ld	-5800(ix), l
	ld	-5799(ix), h
	ld	l, -5800(ix)
	ld	h, -5799(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73879
	ld	hl, #0
	jp	__cmp_e_38620
__cmp_t_73879:
	ld	hl, #1
__cmp_e_38620:
	dec	sp
	dec	sp
	ld	-5802(ix), l
	ld	-5801(ix), h
	ld	l, -5802(ix)
	ld	h, -5801(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1237
	jp	__xcc_L1238
__xcc_L1238:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5804(ix), l
	ld	-5803(ix), h
	ld	l, -5804(ix)
	ld	h, -5803(ix)
	dec	sp
	dec	sp
	ld	-5806(ix), l
	ld	-5805(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5808(ix), l
	ld	-5807(ix), h
	ld	l, -5806(ix)
	ld	h, -5805(ix)
	push	hl
	ld	l, -5808(ix)
	ld	h, -5807(ix)
	ld	b, l
	pop	hl
__shift_3941:
	ld	a, b
	or	a, a
	jp	z, __sdone_3293
	add	hl, hl
	djnz	__shift_3941
__sdone_3293:
	dec	sp
	dec	sp
	ld	-5810(ix), l
	ld	-5809(ix), h
	ld	l, -5810(ix)
	ld	h, -5809(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5812(ix), l
	ld	-5811(ix), h
	ld	l, -5812(ix)
	ld	h, -5811(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22924
	ld	hl, #0
	jp	__cmp_e_86628
__cmp_t_22924:
	ld	hl, #1
__cmp_e_86628:
	dec	sp
	dec	sp
	ld	-5814(ix), l
	ld	-5813(ix), h
	ld	l, -5814(ix)
	ld	h, -5813(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17135
	ld	hl, #0
	jp	__cmp_e_87708
__cmp_t_17135:
	ld	hl, #1
__cmp_e_87708:
	dec	sp
	dec	sp
	ld	-5816(ix), l
	ld	-5815(ix), h
	jp	__xcc_L1239
__xcc_L1237:
	ld	hl, #1
	ld	-5816(ix), l
	ld	-5815(ix), h
__xcc_L1239:
	ld	l, -5816(ix)
	ld	h, -5815(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1234
	jp	__xcc_L1235
__xcc_L1234:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5818(ix), l
	ld	-5817(ix), h
	ld	l, -5818(ix)
	ld	h, -5817(ix)
	dec	sp
	dec	sp
	ld	-5820(ix), l
	ld	-5819(ix), h
	jp	__xcc_L1236
__xcc_L1235:
	ld	hl, #1
	ld	-5820(ix), l
	ld	-5819(ix), h
__xcc_L1236:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5822(ix), l
	ld	-5821(ix), h
	.globl __mul16
	ld	l, -5822(ix)
	ld	h, -5821(ix)
	push	hl
	ld	l, -5820(ix)
	ld	h, -5819(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5824(ix), l
	ld	-5823(ix), h
	ld	l, -5824(ix)
	ld	h, -5823(ix)
	push	hl
	ld	l, -5790(ix)
	ld	h, -5789(ix)
	push	hl
	ld	l, -5764(ix)
	ld	h, -5763(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1240
	dec	sp
	dec	sp
	ld	-5826(ix), l
	ld	-5825(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5828(ix), l
	ld	-5827(ix), h
	ld	l, -5828(ix)
	ld	h, -5827(ix)
	dec	sp
	dec	sp
	ld	-5830(ix), l
	ld	-5829(ix), h
	ld	l, -5830(ix)
	ld	h, -5829(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_83162
	ld	hl, #0
	jp	__cmp_e_56942
__cmp_t_83162:
	ld	hl, #1
__cmp_e_56942:
	dec	sp
	dec	sp
	ld	-5832(ix), l
	ld	-5831(ix), h
	ld	l, -5832(ix)
	ld	h, -5831(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_53870
	ld	hl, #0
	jp	__cmp_e_1996
__cmp_t_53870:
	ld	hl, #1
__cmp_e_1996:
	dec	sp
	dec	sp
	ld	-5834(ix), l
	ld	-5833(ix), h
	ld	l, -5834(ix)
	ld	h, -5833(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1244
	jp	__xcc_L1245
__xcc_L1245:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5836(ix), l
	ld	-5835(ix), h
	ld	l, -5836(ix)
	ld	h, -5835(ix)
	dec	sp
	dec	sp
	ld	-5838(ix), l
	ld	-5837(ix), h
	ld	l, -5838(ix)
	ld	h, -5837(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5840(ix), l
	ld	-5839(ix), h
	ld	l, -5840(ix)
	ld	h, -5839(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_54163
	ld	hl, #0
	jp	__cmp_e_7466
__cmp_t_54163:
	ld	hl, #1
__cmp_e_7466:
	dec	sp
	dec	sp
	ld	-5842(ix), l
	ld	-5841(ix), h
	ld	l, -5842(ix)
	ld	h, -5841(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97811
	ld	hl, #0
	jp	__cmp_e_98500
__cmp_t_97811:
	ld	hl, #1
__cmp_e_98500:
	dec	sp
	dec	sp
	ld	-5844(ix), l
	ld	-5843(ix), h
	jp	__xcc_L1246
__xcc_L1244:
	ld	hl, #1
	ld	-5844(ix), l
	ld	-5843(ix), h
__xcc_L1246:
	ld	l, -5844(ix)
	ld	h, -5843(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1241
	jp	__xcc_L1242
__xcc_L1241:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5846(ix), l
	ld	-5845(ix), h
	ld	l, -5846(ix)
	ld	h, -5845(ix)
	dec	sp
	dec	sp
	ld	-5848(ix), l
	ld	-5847(ix), h
	jp	__xcc_L1243
__xcc_L1242:
	ld	hl, #1
	ld	-5848(ix), l
	ld	-5847(ix), h
__xcc_L1243:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5850(ix), l
	ld	-5849(ix), h
	.globl __mul16
	ld	l, -5850(ix)
	ld	h, -5849(ix)
	push	hl
	ld	l, -5848(ix)
	ld	h, -5847(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5852(ix), l
	ld	-5851(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5854(ix), l
	ld	-5853(ix), h
	ld	l, -5854(ix)
	ld	h, -5853(ix)
	dec	sp
	dec	sp
	ld	-5856(ix), l
	ld	-5855(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5858(ix), l
	ld	-5857(ix), h
	ld	l, -5856(ix)
	ld	h, -5855(ix)
	push	hl
	ld	l, -5858(ix)
	ld	h, -5857(ix)
	ld	b, l
	pop	hl
__shift_2515:
	ld	a, b
	or	a, a
	jp	z, __sdone_6487
	add	hl, hl
	djnz	__shift_2515
__sdone_6487:
	dec	sp
	dec	sp
	ld	-5860(ix), l
	ld	-5859(ix), h
	ld	l, -5860(ix)
	ld	h, -5859(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_26234
	ld	hl, #0
	jp	__cmp_e_83332
__cmp_t_26234:
	ld	hl, #1
__cmp_e_83332:
	dec	sp
	dec	sp
	ld	-5862(ix), l
	ld	-5861(ix), h
	ld	l, -5862(ix)
	ld	h, -5861(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_82290
	ld	hl, #0
	jp	__cmp_e_45950
__cmp_t_82290:
	ld	hl, #1
__cmp_e_45950:
	dec	sp
	dec	sp
	ld	-5864(ix), l
	ld	-5863(ix), h
	ld	l, -5864(ix)
	ld	h, -5863(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1250
	jp	__xcc_L1251
__xcc_L1251:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5866(ix), l
	ld	-5865(ix), h
	ld	l, -5866(ix)
	ld	h, -5865(ix)
	dec	sp
	dec	sp
	ld	-5868(ix), l
	ld	-5867(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5870(ix), l
	ld	-5869(ix), h
	ld	l, -5868(ix)
	ld	h, -5867(ix)
	push	hl
	ld	l, -5870(ix)
	ld	h, -5869(ix)
	ld	b, l
	pop	hl
__shift_8693:
	ld	a, b
	or	a, a
	jp	z, __sdone_1633
	add	hl, hl
	djnz	__shift_8693
__sdone_1633:
	dec	sp
	dec	sp
	ld	-5872(ix), l
	ld	-5871(ix), h
	ld	l, -5872(ix)
	ld	h, -5871(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5874(ix), l
	ld	-5873(ix), h
	ld	l, -5874(ix)
	ld	h, -5873(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_25339
	ld	hl, #0
	jp	__cmp_e_62880
__cmp_t_25339:
	ld	hl, #1
__cmp_e_62880:
	dec	sp
	dec	sp
	ld	-5876(ix), l
	ld	-5875(ix), h
	ld	l, -5876(ix)
	ld	h, -5875(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_27494
	ld	hl, #0
	jp	__cmp_e_2293
__cmp_t_27494:
	ld	hl, #1
__cmp_e_2293:
	dec	sp
	dec	sp
	ld	-5878(ix), l
	ld	-5877(ix), h
	jp	__xcc_L1252
__xcc_L1250:
	ld	hl, #1
	ld	-5878(ix), l
	ld	-5877(ix), h
__xcc_L1252:
	ld	l, -5878(ix)
	ld	h, -5877(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1247
	jp	__xcc_L1248
__xcc_L1247:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5880(ix), l
	ld	-5879(ix), h
	ld	l, -5880(ix)
	ld	h, -5879(ix)
	dec	sp
	dec	sp
	ld	-5882(ix), l
	ld	-5881(ix), h
	jp	__xcc_L1249
__xcc_L1248:
	ld	hl, #1
	ld	-5882(ix), l
	ld	-5881(ix), h
__xcc_L1249:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5884(ix), l
	ld	-5883(ix), h
	.globl __mul16
	ld	l, -5884(ix)
	ld	h, -5883(ix)
	push	hl
	ld	l, -5882(ix)
	ld	h, -5881(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5886(ix), l
	ld	-5885(ix), h
	ld	l, -5886(ix)
	ld	h, -5885(ix)
	push	hl
	ld	l, -5852(ix)
	ld	h, -5851(ix)
	push	hl
	ld	l, -5826(ix)
	ld	h, -5825(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1225:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1224
	jp	__xcc_L1226
__xcc_L1226:
__xcc_L1253:
	ld	hl, #__str_1256
	dec	sp
	dec	sp
	ld	-5888(ix), l
	ld	-5887(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5890(ix), l
	ld	-5889(ix), h
	ld	l, -5890(ix)
	ld	h, -5889(ix)
	dec	sp
	dec	sp
	ld	-5892(ix), l
	ld	-5891(ix), h
	ld	l, -5892(ix)
	ld	h, -5891(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85213
	ld	hl, #0
	jp	__cmp_e_32854
__cmp_t_85213:
	ld	hl, #1
__cmp_e_32854:
	dec	sp
	dec	sp
	ld	-5894(ix), l
	ld	-5893(ix), h
	ld	l, -5894(ix)
	ld	h, -5893(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66382
	ld	hl, #0
	jp	__cmp_e_75444
__cmp_t_66382:
	ld	hl, #1
__cmp_e_75444:
	dec	sp
	dec	sp
	ld	-5896(ix), l
	ld	-5895(ix), h
	ld	l, -5896(ix)
	ld	h, -5895(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1260
	jp	__xcc_L1261
__xcc_L1261:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5898(ix), l
	ld	-5897(ix), h
	ld	l, -5898(ix)
	ld	h, -5897(ix)
	dec	sp
	dec	sp
	ld	-5900(ix), l
	ld	-5899(ix), h
	ld	l, -5900(ix)
	ld	h, -5899(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5902(ix), l
	ld	-5901(ix), h
	ld	l, -5902(ix)
	ld	h, -5901(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71475
	ld	hl, #0
	jp	__cmp_e_30323
__cmp_t_71475:
	ld	hl, #1
__cmp_e_30323:
	dec	sp
	dec	sp
	ld	-5904(ix), l
	ld	-5903(ix), h
	ld	l, -5904(ix)
	ld	h, -5903(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98738
	ld	hl, #0
	jp	__cmp_e_10751
__cmp_t_98738:
	ld	hl, #1
__cmp_e_10751:
	dec	sp
	dec	sp
	ld	-5906(ix), l
	ld	-5905(ix), h
	jp	__xcc_L1262
__xcc_L1260:
	ld	hl, #1
	ld	-5906(ix), l
	ld	-5905(ix), h
__xcc_L1262:
	ld	l, -5906(ix)
	ld	h, -5905(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1257
	jp	__xcc_L1258
__xcc_L1257:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5908(ix), l
	ld	-5907(ix), h
	ld	l, -5908(ix)
	ld	h, -5907(ix)
	dec	sp
	dec	sp
	ld	-5910(ix), l
	ld	-5909(ix), h
	jp	__xcc_L1259
__xcc_L1258:
	ld	hl, #1
	ld	-5910(ix), l
	ld	-5909(ix), h
__xcc_L1259:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5912(ix), l
	ld	-5911(ix), h
	.globl __mul16
	ld	l, -5912(ix)
	ld	h, -5911(ix)
	push	hl
	ld	l, -5910(ix)
	ld	h, -5909(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5914(ix), l
	ld	-5913(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5916(ix), l
	ld	-5915(ix), h
	ld	l, -5916(ix)
	ld	h, -5915(ix)
	dec	sp
	dec	sp
	ld	-5918(ix), l
	ld	-5917(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5920(ix), l
	ld	-5919(ix), h
	ld	l, -5918(ix)
	ld	h, -5917(ix)
	push	hl
	ld	l, -5920(ix)
	ld	h, -5919(ix)
	ld	b, l
	pop	hl
__shift_6951:
	ld	a, b
	or	a, a
	jp	z, __sdone_5873
	add	hl, hl
	djnz	__shift_6951
__sdone_5873:
	dec	sp
	dec	sp
	ld	-5922(ix), l
	ld	-5921(ix), h
	ld	l, -5922(ix)
	ld	h, -5921(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_14811
	ld	hl, #0
	jp	__cmp_e_16465
__cmp_t_14811:
	ld	hl, #1
__cmp_e_16465:
	dec	sp
	dec	sp
	ld	-5924(ix), l
	ld	-5923(ix), h
	ld	l, -5924(ix)
	ld	h, -5923(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89168
	ld	hl, #0
	jp	__cmp_e_68681
__cmp_t_89168:
	ld	hl, #1
__cmp_e_68681:
	dec	sp
	dec	sp
	ld	-5926(ix), l
	ld	-5925(ix), h
	ld	l, -5926(ix)
	ld	h, -5925(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1266
	jp	__xcc_L1267
__xcc_L1267:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5928(ix), l
	ld	-5927(ix), h
	ld	l, -5928(ix)
	ld	h, -5927(ix)
	dec	sp
	dec	sp
	ld	-5930(ix), l
	ld	-5929(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5932(ix), l
	ld	-5931(ix), h
	ld	l, -5930(ix)
	ld	h, -5929(ix)
	push	hl
	ld	l, -5932(ix)
	ld	h, -5931(ix)
	ld	b, l
	pop	hl
__shift_4813:
	ld	a, b
	or	a, a
	jp	z, __sdone_9683
	add	hl, hl
	djnz	__shift_4813
__sdone_9683:
	dec	sp
	dec	sp
	ld	-5934(ix), l
	ld	-5933(ix), h
	ld	l, -5934(ix)
	ld	h, -5933(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5936(ix), l
	ld	-5935(ix), h
	ld	l, -5936(ix)
	ld	h, -5935(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92499
	ld	hl, #0
	jp	__cmp_e_48977
__cmp_t_92499:
	ld	hl, #1
__cmp_e_48977:
	dec	sp
	dec	sp
	ld	-5938(ix), l
	ld	-5937(ix), h
	ld	l, -5938(ix)
	ld	h, -5937(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74535
	ld	hl, #0
	jp	__cmp_e_65014
__cmp_t_74535:
	ld	hl, #1
__cmp_e_65014:
	dec	sp
	dec	sp
	ld	-5940(ix), l
	ld	-5939(ix), h
	jp	__xcc_L1268
__xcc_L1266:
	ld	hl, #1
	ld	-5940(ix), l
	ld	-5939(ix), h
__xcc_L1268:
	ld	l, -5940(ix)
	ld	h, -5939(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1263
	jp	__xcc_L1264
__xcc_L1263:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5942(ix), l
	ld	-5941(ix), h
	ld	l, -5942(ix)
	ld	h, -5941(ix)
	dec	sp
	dec	sp
	ld	-5944(ix), l
	ld	-5943(ix), h
	jp	__xcc_L1265
__xcc_L1264:
	ld	hl, #1
	ld	-5944(ix), l
	ld	-5943(ix), h
__xcc_L1265:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5946(ix), l
	ld	-5945(ix), h
	.globl __mul16
	ld	l, -5946(ix)
	ld	h, -5945(ix)
	push	hl
	ld	l, -5944(ix)
	ld	h, -5943(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5948(ix), l
	ld	-5947(ix), h
	ld	l, -5948(ix)
	ld	h, -5947(ix)
	push	hl
	ld	l, -5914(ix)
	ld	h, -5913(ix)
	push	hl
	ld	l, -5888(ix)
	ld	h, -5887(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1269
	dec	sp
	dec	sp
	ld	-5950(ix), l
	ld	-5949(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5952(ix), l
	ld	-5951(ix), h
	ld	l, -5952(ix)
	ld	h, -5951(ix)
	dec	sp
	dec	sp
	ld	-5954(ix), l
	ld	-5953(ix), h
	ld	l, -5954(ix)
	ld	h, -5953(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_15464
	ld	hl, #0
	jp	__cmp_e_769
__cmp_t_15464:
	ld	hl, #1
__cmp_e_769:
	dec	sp
	dec	sp
	ld	-5956(ix), l
	ld	-5955(ix), h
	ld	l, -5956(ix)
	ld	h, -5955(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_48346
	ld	hl, #0
	jp	__cmp_e_14107
__cmp_t_48346:
	ld	hl, #1
__cmp_e_14107:
	dec	sp
	dec	sp
	ld	-5958(ix), l
	ld	-5957(ix), h
	ld	l, -5958(ix)
	ld	h, -5957(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1273
	jp	__xcc_L1274
__xcc_L1274:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5960(ix), l
	ld	-5959(ix), h
	ld	l, -5960(ix)
	ld	h, -5959(ix)
	dec	sp
	dec	sp
	ld	-5962(ix), l
	ld	-5961(ix), h
	ld	l, -5962(ix)
	ld	h, -5961(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5964(ix), l
	ld	-5963(ix), h
	ld	l, -5964(ix)
	ld	h, -5963(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63072
	ld	hl, #0
	jp	__cmp_e_73391
__cmp_t_63072:
	ld	hl, #1
__cmp_e_73391:
	dec	sp
	dec	sp
	ld	-5966(ix), l
	ld	-5965(ix), h
	ld	l, -5966(ix)
	ld	h, -5965(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65740
	ld	hl, #0
	jp	__cmp_e_88411
__cmp_t_65740:
	ld	hl, #1
__cmp_e_88411:
	dec	sp
	dec	sp
	ld	-5968(ix), l
	ld	-5967(ix), h
	jp	__xcc_L1275
__xcc_L1273:
	ld	hl, #1
	ld	-5968(ix), l
	ld	-5967(ix), h
__xcc_L1275:
	ld	l, -5968(ix)
	ld	h, -5967(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1270
	jp	__xcc_L1271
__xcc_L1270:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5970(ix), l
	ld	-5969(ix), h
	ld	l, -5970(ix)
	ld	h, -5969(ix)
	dec	sp
	dec	sp
	ld	-5972(ix), l
	ld	-5971(ix), h
	jp	__xcc_L1272
__xcc_L1271:
	ld	hl, #1
	ld	-5972(ix), l
	ld	-5971(ix), h
__xcc_L1272:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-5974(ix), l
	ld	-5973(ix), h
	.globl __mul16
	ld	l, -5974(ix)
	ld	h, -5973(ix)
	push	hl
	ld	l, -5972(ix)
	ld	h, -5971(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-5976(ix), l
	ld	-5975(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5978(ix), l
	ld	-5977(ix), h
	ld	l, -5978(ix)
	ld	h, -5977(ix)
	dec	sp
	dec	sp
	ld	-5980(ix), l
	ld	-5979(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5982(ix), l
	ld	-5981(ix), h
	ld	l, -5980(ix)
	ld	h, -5979(ix)
	push	hl
	ld	l, -5982(ix)
	ld	h, -5981(ix)
	ld	b, l
	pop	hl
__shift_2623:
	ld	a, b
	or	a, a
	jp	z, __sdone_9587
	add	hl, hl
	djnz	__shift_2623
__sdone_9587:
	dec	sp
	dec	sp
	ld	-5984(ix), l
	ld	-5983(ix), h
	ld	l, -5984(ix)
	ld	h, -5983(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_7057
	ld	hl, #0
	jp	__cmp_e_37836
__cmp_t_7057:
	ld	hl, #1
__cmp_e_37836:
	dec	sp
	dec	sp
	ld	-5986(ix), l
	ld	-5985(ix), h
	ld	l, -5986(ix)
	ld	h, -5985(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58793
	ld	hl, #0
	jp	__cmp_e_73439
__cmp_t_58793:
	ld	hl, #1
__cmp_e_73439:
	dec	sp
	dec	sp
	ld	-5988(ix), l
	ld	-5987(ix), h
	ld	l, -5988(ix)
	ld	h, -5987(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1279
	jp	__xcc_L1280
__xcc_L1280:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5990(ix), l
	ld	-5989(ix), h
	ld	l, -5990(ix)
	ld	h, -5989(ix)
	dec	sp
	dec	sp
	ld	-5992(ix), l
	ld	-5991(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-5994(ix), l
	ld	-5993(ix), h
	ld	l, -5992(ix)
	ld	h, -5991(ix)
	push	hl
	ld	l, -5994(ix)
	ld	h, -5993(ix)
	ld	b, l
	pop	hl
__shift_3281:
	ld	a, b
	or	a, a
	jp	z, __sdone_6620
	add	hl, hl
	djnz	__shift_3281
__sdone_6620:
	dec	sp
	dec	sp
	ld	-5996(ix), l
	ld	-5995(ix), h
	ld	l, -5996(ix)
	ld	h, -5995(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-5998(ix), l
	ld	-5997(ix), h
	ld	l, -5998(ix)
	ld	h, -5997(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20114
	ld	hl, #0
	jp	__cmp_e_28371
__cmp_t_20114:
	ld	hl, #1
__cmp_e_28371:
	dec	sp
	dec	sp
	ld	-6000(ix), l
	ld	-5999(ix), h
	ld	l, -6000(ix)
	ld	h, -5999(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_57371
	ld	hl, #0
	jp	__cmp_e_53418
__cmp_t_57371:
	ld	hl, #1
__cmp_e_53418:
	dec	sp
	dec	sp
	ld	-6002(ix), l
	ld	-6001(ix), h
	jp	__xcc_L1281
__xcc_L1279:
	ld	hl, #1
	ld	-6002(ix), l
	ld	-6001(ix), h
__xcc_L1281:
	ld	l, -6002(ix)
	ld	h, -6001(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1276
	jp	__xcc_L1277
__xcc_L1276:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6004(ix), l
	ld	-6003(ix), h
	ld	l, -6004(ix)
	ld	h, -6003(ix)
	dec	sp
	dec	sp
	ld	-6006(ix), l
	ld	-6005(ix), h
	jp	__xcc_L1278
__xcc_L1277:
	ld	hl, #1
	ld	-6006(ix), l
	ld	-6005(ix), h
__xcc_L1278:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6008(ix), l
	ld	-6007(ix), h
	.globl __mul16
	ld	l, -6008(ix)
	ld	h, -6007(ix)
	push	hl
	ld	l, -6006(ix)
	ld	h, -6005(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6010(ix), l
	ld	-6009(ix), h
	ld	l, -6010(ix)
	ld	h, -6009(ix)
	push	hl
	ld	l, -5976(ix)
	ld	h, -5975(ix)
	push	hl
	ld	l, -5950(ix)
	ld	h, -5949(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1254:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1253
	jp	__xcc_L1255
__xcc_L1255:
__xcc_L1282:
	ld	hl, #__str_1285
	dec	sp
	dec	sp
	ld	-6012(ix), l
	ld	-6011(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6014(ix), l
	ld	-6013(ix), h
	ld	l, -6014(ix)
	ld	h, -6013(ix)
	dec	sp
	dec	sp
	ld	-6016(ix), l
	ld	-6015(ix), h
	ld	l, -6016(ix)
	ld	h, -6015(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60596
	ld	hl, #0
	jp	__cmp_e_88534
__cmp_t_60596:
	ld	hl, #1
__cmp_e_88534:
	dec	sp
	dec	sp
	ld	-6018(ix), l
	ld	-6017(ix), h
	ld	l, -6018(ix)
	ld	h, -6017(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_69883
	ld	hl, #0
	jp	__cmp_e_49764
__cmp_t_69883:
	ld	hl, #1
__cmp_e_49764:
	dec	sp
	dec	sp
	ld	-6020(ix), l
	ld	-6019(ix), h
	ld	l, -6020(ix)
	ld	h, -6019(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1289
	jp	__xcc_L1290
__xcc_L1290:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6022(ix), l
	ld	-6021(ix), h
	ld	l, -6022(ix)
	ld	h, -6021(ix)
	dec	sp
	dec	sp
	ld	-6024(ix), l
	ld	-6023(ix), h
	ld	l, -6024(ix)
	ld	h, -6023(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6026(ix), l
	ld	-6025(ix), h
	ld	l, -6026(ix)
	ld	h, -6025(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73567
	ld	hl, #0
	jp	__cmp_e_4697
__cmp_t_73567:
	ld	hl, #1
__cmp_e_4697:
	dec	sp
	dec	sp
	ld	-6028(ix), l
	ld	-6027(ix), h
	ld	l, -6028(ix)
	ld	h, -6027(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25800
	ld	hl, #0
	jp	__cmp_e_66067
__cmp_t_25800:
	ld	hl, #1
__cmp_e_66067:
	dec	sp
	dec	sp
	ld	-6030(ix), l
	ld	-6029(ix), h
	jp	__xcc_L1291
__xcc_L1289:
	ld	hl, #1
	ld	-6030(ix), l
	ld	-6029(ix), h
__xcc_L1291:
	ld	l, -6030(ix)
	ld	h, -6029(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1286
	jp	__xcc_L1287
__xcc_L1286:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6032(ix), l
	ld	-6031(ix), h
	ld	l, -6032(ix)
	ld	h, -6031(ix)
	dec	sp
	dec	sp
	ld	-6034(ix), l
	ld	-6033(ix), h
	jp	__xcc_L1288
__xcc_L1287:
	ld	hl, #1
	ld	-6034(ix), l
	ld	-6033(ix), h
__xcc_L1288:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6036(ix), l
	ld	-6035(ix), h
	.globl __mul16
	ld	l, -6036(ix)
	ld	h, -6035(ix)
	push	hl
	ld	l, -6034(ix)
	ld	h, -6033(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6038(ix), l
	ld	-6037(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6040(ix), l
	ld	-6039(ix), h
	ld	l, -6040(ix)
	ld	h, -6039(ix)
	dec	sp
	dec	sp
	ld	-6042(ix), l
	ld	-6041(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6046(ix), l
	ld	-6045(ix), h
	ld	l, -6042(ix)
	ld	h, -6041(ix)
	push	hl
	ld	l, -6046(ix)
	ld	h, -6045(ix)
	ld	b, l
	pop	hl
__shift_3674:
	ld	a, b
	or	a, a
	jp	z, __sdone_335
	add	hl, hl
	djnz	__shift_3674
__sdone_335:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6050(ix), l
	ld	-6049(ix), h
	ld	l, -6050(ix)
	ld	h, -6049(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47433
	ld	hl, #0
	jp	__cmp_e_85490
__cmp_t_47433:
	ld	hl, #1
__cmp_e_85490:
	dec	sp
	dec	sp
	ld	-6052(ix), l
	ld	-6051(ix), h
	ld	l, -6052(ix)
	ld	h, -6051(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_17457
	ld	hl, #0
	jp	__cmp_e_12132
__cmp_t_17457:
	ld	hl, #1
__cmp_e_12132:
	dec	sp
	dec	sp
	ld	-6054(ix), l
	ld	-6053(ix), h
	ld	l, -6054(ix)
	ld	h, -6053(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1295
	jp	__xcc_L1296
__xcc_L1296:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6056(ix), l
	ld	-6055(ix), h
	ld	l, -6056(ix)
	ld	h, -6055(ix)
	dec	sp
	dec	sp
	ld	-6058(ix), l
	ld	-6057(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6062(ix), l
	ld	-6061(ix), h
	ld	l, -6058(ix)
	ld	h, -6057(ix)
	push	hl
	ld	l, -6062(ix)
	ld	h, -6061(ix)
	ld	b, l
	pop	hl
__shift_5949:
	ld	a, b
	or	a, a
	jp	z, __sdone_529
	add	hl, hl
	djnz	__shift_5949
__sdone_529:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6066(ix), l
	ld	-6065(ix), h
	ld	l, -6066(ix)
	ld	h, -6065(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6068(ix), l
	ld	-6067(ix), h
	ld	l, -6068(ix)
	ld	h, -6067(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85523
	ld	hl, #0
	jp	__cmp_e_81690
__cmp_t_85523:
	ld	hl, #1
__cmp_e_81690:
	dec	sp
	dec	sp
	ld	-6070(ix), l
	ld	-6069(ix), h
	ld	l, -6070(ix)
	ld	h, -6069(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85292
	ld	hl, #0
	jp	__cmp_e_38147
__cmp_t_85292:
	ld	hl, #1
__cmp_e_38147:
	dec	sp
	dec	sp
	ld	-6072(ix), l
	ld	-6071(ix), h
	jp	__xcc_L1297
__xcc_L1295:
	ld	hl, #1
	ld	-6072(ix), l
	ld	-6071(ix), h
__xcc_L1297:
	ld	l, -6072(ix)
	ld	h, -6071(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1292
	jp	__xcc_L1293
__xcc_L1292:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6074(ix), l
	ld	-6073(ix), h
	ld	l, -6074(ix)
	ld	h, -6073(ix)
	dec	sp
	dec	sp
	ld	-6076(ix), l
	ld	-6075(ix), h
	jp	__xcc_L1294
__xcc_L1293:
	ld	hl, #1
	ld	-6076(ix), l
	ld	-6075(ix), h
__xcc_L1294:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6078(ix), l
	ld	-6077(ix), h
	.globl __mul16
	ld	l, -6078(ix)
	ld	h, -6077(ix)
	push	hl
	ld	l, -6076(ix)
	ld	h, -6075(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6080(ix), l
	ld	-6079(ix), h
	ld	l, -6080(ix)
	ld	h, -6079(ix)
	push	hl
	ld	l, -6038(ix)
	ld	h, -6037(ix)
	push	hl
	ld	l, -6012(ix)
	ld	h, -6011(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1298
	dec	sp
	dec	sp
	ld	-6082(ix), l
	ld	-6081(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6084(ix), l
	ld	-6083(ix), h
	ld	l, -6084(ix)
	ld	h, -6083(ix)
	dec	sp
	dec	sp
	ld	-6086(ix), l
	ld	-6085(ix), h
	ld	l, -6086(ix)
	ld	h, -6085(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_7629
	ld	hl, #0
	jp	__cmp_e_92349
__cmp_t_7629:
	ld	hl, #1
__cmp_e_92349:
	dec	sp
	dec	sp
	ld	-6088(ix), l
	ld	-6087(ix), h
	ld	l, -6088(ix)
	ld	h, -6087(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92335
	ld	hl, #0
	jp	__cmp_e_66422
__cmp_t_92335:
	ld	hl, #1
__cmp_e_66422:
	dec	sp
	dec	sp
	ld	-6090(ix), l
	ld	-6089(ix), h
	ld	l, -6090(ix)
	ld	h, -6089(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1302
	jp	__xcc_L1303
__xcc_L1303:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6092(ix), l
	ld	-6091(ix), h
	ld	l, -6092(ix)
	ld	h, -6091(ix)
	dec	sp
	dec	sp
	ld	-6094(ix), l
	ld	-6093(ix), h
	ld	l, -6094(ix)
	ld	h, -6093(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6096(ix), l
	ld	-6095(ix), h
	ld	l, -6096(ix)
	ld	h, -6095(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_82140
	ld	hl, #0
	jp	__cmp_e_21968
__cmp_t_82140:
	ld	hl, #1
__cmp_e_21968:
	dec	sp
	dec	sp
	ld	-6098(ix), l
	ld	-6097(ix), h
	ld	l, -6098(ix)
	ld	h, -6097(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13043
	ld	hl, #0
	jp	__cmp_e_2255
__cmp_t_13043:
	ld	hl, #1
__cmp_e_2255:
	dec	sp
	dec	sp
	ld	-6100(ix), l
	ld	-6099(ix), h
	jp	__xcc_L1304
__xcc_L1302:
	ld	hl, #1
	ld	-6100(ix), l
	ld	-6099(ix), h
__xcc_L1304:
	ld	l, -6100(ix)
	ld	h, -6099(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1299
	jp	__xcc_L1300
__xcc_L1299:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6102(ix), l
	ld	-6101(ix), h
	ld	l, -6102(ix)
	ld	h, -6101(ix)
	dec	sp
	dec	sp
	ld	-6104(ix), l
	ld	-6103(ix), h
	jp	__xcc_L1301
__xcc_L1300:
	ld	hl, #1
	ld	-6104(ix), l
	ld	-6103(ix), h
__xcc_L1301:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6106(ix), l
	ld	-6105(ix), h
	.globl __mul16
	ld	l, -6106(ix)
	ld	h, -6105(ix)
	push	hl
	ld	l, -6104(ix)
	ld	h, -6103(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6108(ix), l
	ld	-6107(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6110(ix), l
	ld	-6109(ix), h
	ld	l, -6110(ix)
	ld	h, -6109(ix)
	dec	sp
	dec	sp
	ld	-6112(ix), l
	ld	-6111(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6116(ix), l
	ld	-6115(ix), h
	ld	l, -6112(ix)
	ld	h, -6111(ix)
	push	hl
	ld	l, -6116(ix)
	ld	h, -6115(ix)
	ld	b, l
	pop	hl
__shift_339:
	ld	a, b
	or	a, a
	jp	z, __sdone_6766
	add	hl, hl
	djnz	__shift_339
__sdone_6766:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6120(ix), l
	ld	-6119(ix), h
	ld	l, -6120(ix)
	ld	h, -6119(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72025
	ld	hl, #0
	jp	__cmp_e_10936
__cmp_t_72025:
	ld	hl, #1
__cmp_e_10936:
	dec	sp
	dec	sp
	ld	-6122(ix), l
	ld	-6121(ix), h
	ld	l, -6122(ix)
	ld	h, -6121(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_91653
	ld	hl, #0
	jp	__cmp_e_58260
__cmp_t_91653:
	ld	hl, #1
__cmp_e_58260:
	dec	sp
	dec	sp
	ld	-6124(ix), l
	ld	-6123(ix), h
	ld	l, -6124(ix)
	ld	h, -6123(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1308
	jp	__xcc_L1309
__xcc_L1309:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6126(ix), l
	ld	-6125(ix), h
	ld	l, -6126(ix)
	ld	h, -6125(ix)
	dec	sp
	dec	sp
	ld	-6128(ix), l
	ld	-6127(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6132(ix), l
	ld	-6131(ix), h
	ld	l, -6128(ix)
	ld	h, -6127(ix)
	push	hl
	ld	l, -6132(ix)
	ld	h, -6131(ix)
	ld	b, l
	pop	hl
__shift_7052:
	ld	a, b
	or	a, a
	jp	z, __sdone_5220
	add	hl, hl
	djnz	__shift_7052
__sdone_5220:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6136(ix), l
	ld	-6135(ix), h
	ld	l, -6136(ix)
	ld	h, -6135(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6138(ix), l
	ld	-6137(ix), h
	ld	l, -6138(ix)
	ld	h, -6137(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_62957
	ld	hl, #0
	jp	__cmp_e_19204
__cmp_t_62957:
	ld	hl, #1
__cmp_e_19204:
	dec	sp
	dec	sp
	ld	-6140(ix), l
	ld	-6139(ix), h
	ld	l, -6140(ix)
	ld	h, -6139(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_31287
	ld	hl, #0
	jp	__cmp_e_32983
__cmp_t_31287:
	ld	hl, #1
__cmp_e_32983:
	dec	sp
	dec	sp
	ld	-6142(ix), l
	ld	-6141(ix), h
	jp	__xcc_L1310
__xcc_L1308:
	ld	hl, #1
	ld	-6142(ix), l
	ld	-6141(ix), h
__xcc_L1310:
	ld	l, -6142(ix)
	ld	h, -6141(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1305
	jp	__xcc_L1306
__xcc_L1305:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6144(ix), l
	ld	-6143(ix), h
	ld	l, -6144(ix)
	ld	h, -6143(ix)
	dec	sp
	dec	sp
	ld	-6146(ix), l
	ld	-6145(ix), h
	jp	__xcc_L1307
__xcc_L1306:
	ld	hl, #1
	ld	-6146(ix), l
	ld	-6145(ix), h
__xcc_L1307:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6148(ix), l
	ld	-6147(ix), h
	.globl __mul16
	ld	l, -6148(ix)
	ld	h, -6147(ix)
	push	hl
	ld	l, -6146(ix)
	ld	h, -6145(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6150(ix), l
	ld	-6149(ix), h
	ld	l, -6150(ix)
	ld	h, -6149(ix)
	push	hl
	ld	l, -6108(ix)
	ld	h, -6107(ix)
	push	hl
	ld	l, -6082(ix)
	ld	h, -6081(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1283:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1282
	jp	__xcc_L1284
__xcc_L1284:
__xcc_L1311:
	ld	hl, #__str_1314
	dec	sp
	dec	sp
	ld	-6152(ix), l
	ld	-6151(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6154(ix), l
	ld	-6153(ix), h
	ld	l, -6154(ix)
	ld	h, -6153(ix)
	dec	sp
	dec	sp
	ld	-6156(ix), l
	ld	-6155(ix), h
	ld	l, -6156(ix)
	ld	h, -6155(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_19540
	ld	hl, #0
	jp	__cmp_e_78721
__cmp_t_19540:
	ld	hl, #1
__cmp_e_78721:
	dec	sp
	dec	sp
	ld	-6158(ix), l
	ld	-6157(ix), h
	ld	l, -6158(ix)
	ld	h, -6157(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34826
	ld	hl, #0
	jp	__cmp_e_36997
__cmp_t_34826:
	ld	hl, #1
__cmp_e_36997:
	dec	sp
	dec	sp
	ld	-6160(ix), l
	ld	-6159(ix), h
	ld	l, -6160(ix)
	ld	h, -6159(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1318
	jp	__xcc_L1319
__xcc_L1319:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6162(ix), l
	ld	-6161(ix), h
	ld	l, -6162(ix)
	ld	h, -6161(ix)
	dec	sp
	dec	sp
	ld	-6164(ix), l
	ld	-6163(ix), h
	ld	l, -6164(ix)
	ld	h, -6163(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6166(ix), l
	ld	-6165(ix), h
	ld	l, -6166(ix)
	ld	h, -6165(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_7205
	ld	hl, #0
	jp	__cmp_e_67127
__cmp_t_7205:
	ld	hl, #1
__cmp_e_67127:
	dec	sp
	dec	sp
	ld	-6168(ix), l
	ld	-6167(ix), h
	ld	l, -6168(ix)
	ld	h, -6167(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33878
	ld	hl, #0
	jp	__cmp_e_92728
__cmp_t_33878:
	ld	hl, #1
__cmp_e_92728:
	dec	sp
	dec	sp
	ld	-6170(ix), l
	ld	-6169(ix), h
	jp	__xcc_L1320
__xcc_L1318:
	ld	hl, #1
	ld	-6170(ix), l
	ld	-6169(ix), h
__xcc_L1320:
	ld	l, -6170(ix)
	ld	h, -6169(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1315
	jp	__xcc_L1316
__xcc_L1315:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6172(ix), l
	ld	-6171(ix), h
	ld	l, -6172(ix)
	ld	h, -6171(ix)
	dec	sp
	dec	sp
	ld	-6174(ix), l
	ld	-6173(ix), h
	jp	__xcc_L1317
__xcc_L1316:
	ld	hl, #1
	ld	-6174(ix), l
	ld	-6173(ix), h
__xcc_L1317:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6176(ix), l
	ld	-6175(ix), h
	.globl __mul16
	ld	l, -6176(ix)
	ld	h, -6175(ix)
	push	hl
	ld	l, -6174(ix)
	ld	h, -6173(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6178(ix), l
	ld	-6177(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6180(ix), l
	ld	-6179(ix), h
	ld	l, -6180(ix)
	ld	h, -6179(ix)
	dec	sp
	dec	sp
	ld	-6182(ix), l
	ld	-6181(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6190(ix), l
	ld	-6189(ix), h
	ld	l, -6182(ix)
	ld	h, -6181(ix)
	push	hl
	ld	l, -6190(ix)
	ld	h, -6189(ix)
	ld	b, l
	pop	hl
__shift_5169:
	ld	a, b
	or	a, a
	jp	z, __sdone_9170
	add	hl, hl
	djnz	__shift_5169
__sdone_9170:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6198(ix), l
	ld	-6197(ix), h
	ld	l, -6198(ix)
	ld	h, -6197(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47227
	ld	hl, #0
	jp	__cmp_e_72798
__cmp_t_47227:
	ld	hl, #1
__cmp_e_72798:
	dec	sp
	dec	sp
	ld	-6200(ix), l
	ld	-6199(ix), h
	ld	l, -6200(ix)
	ld	h, -6199(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_11520
	ld	hl, #0
	jp	__cmp_e_39563
__cmp_t_11520:
	ld	hl, #1
__cmp_e_39563:
	dec	sp
	dec	sp
	ld	-6202(ix), l
	ld	-6201(ix), h
	ld	l, -6202(ix)
	ld	h, -6201(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1324
	jp	__xcc_L1325
__xcc_L1325:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6204(ix), l
	ld	-6203(ix), h
	ld	l, -6204(ix)
	ld	h, -6203(ix)
	dec	sp
	dec	sp
	ld	-6206(ix), l
	ld	-6205(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6214(ix), l
	ld	-6213(ix), h
	ld	l, -6206(ix)
	ld	h, -6205(ix)
	push	hl
	ld	l, -6214(ix)
	ld	h, -6213(ix)
	ld	b, l
	pop	hl
__shift_9221:
	ld	a, b
	or	a, a
	jp	z, __sdone_3660
	add	hl, hl
	djnz	__shift_9221
__sdone_3660:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6222(ix), l
	ld	-6221(ix), h
	ld	l, -6222(ix)
	ld	h, -6221(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6224(ix), l
	ld	-6223(ix), h
	ld	l, -6224(ix)
	ld	h, -6223(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_77883
	ld	hl, #0
	jp	__cmp_e_68616
__cmp_t_77883:
	ld	hl, #1
__cmp_e_68616:
	dec	sp
	dec	sp
	ld	-6226(ix), l
	ld	-6225(ix), h
	ld	l, -6226(ix)
	ld	h, -6225(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_12267
	ld	hl, #0
	jp	__cmp_e_28223
__cmp_t_12267:
	ld	hl, #1
__cmp_e_28223:
	dec	sp
	dec	sp
	ld	-6228(ix), l
	ld	-6227(ix), h
	jp	__xcc_L1326
__xcc_L1324:
	ld	hl, #1
	ld	-6228(ix), l
	ld	-6227(ix), h
__xcc_L1326:
	ld	l, -6228(ix)
	ld	h, -6227(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1321
	jp	__xcc_L1322
__xcc_L1321:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6230(ix), l
	ld	-6229(ix), h
	ld	l, -6230(ix)
	ld	h, -6229(ix)
	dec	sp
	dec	sp
	ld	-6232(ix), l
	ld	-6231(ix), h
	jp	__xcc_L1323
__xcc_L1322:
	ld	hl, #1
	ld	-6232(ix), l
	ld	-6231(ix), h
__xcc_L1323:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-6234(ix), l
	ld	-6233(ix), h
	.globl __mul16
	ld	l, -6234(ix)
	ld	h, -6233(ix)
	push	hl
	ld	l, -6232(ix)
	ld	h, -6231(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6236(ix), l
	ld	-6235(ix), h
	ld	l, -6236(ix)
	ld	h, -6235(ix)
	push	hl
	ld	l, -6178(ix)
	ld	h, -6177(ix)
	push	hl
	ld	l, -6152(ix)
	ld	h, -6151(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1327
	dec	sp
	dec	sp
	ld	-6238(ix), l
	ld	-6237(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6240(ix), l
	ld	-6239(ix), h
	ld	l, -6240(ix)
	ld	h, -6239(ix)
	dec	sp
	dec	sp
	ld	-6242(ix), l
	ld	-6241(ix), h
	ld	l, -6242(ix)
	ld	h, -6241(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_55382
	ld	hl, #0
	jp	__cmp_e_84292
__cmp_t_55382:
	ld	hl, #1
__cmp_e_84292:
	dec	sp
	dec	sp
	ld	-6244(ix), l
	ld	-6243(ix), h
	ld	l, -6244(ix)
	ld	h, -6243(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_55511
	ld	hl, #0
	jp	__cmp_e_47035
__cmp_t_55511:
	ld	hl, #1
__cmp_e_47035:
	dec	sp
	dec	sp
	ld	-6246(ix), l
	ld	-6245(ix), h
	ld	l, -6246(ix)
	ld	h, -6245(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1331
	jp	__xcc_L1332
__xcc_L1332:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6248(ix), l
	ld	-6247(ix), h
	ld	l, -6248(ix)
	ld	h, -6247(ix)
	dec	sp
	dec	sp
	ld	-6250(ix), l
	ld	-6249(ix), h
	ld	l, -6250(ix)
	ld	h, -6249(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6252(ix), l
	ld	-6251(ix), h
	ld	l, -6252(ix)
	ld	h, -6251(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_42553
	ld	hl, #0
	jp	__cmp_e_32563
__cmp_t_42553:
	ld	hl, #1
__cmp_e_32563:
	dec	sp
	dec	sp
	ld	-6254(ix), l
	ld	-6253(ix), h
	ld	l, -6254(ix)
	ld	h, -6253(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_28608
	ld	hl, #0
	jp	__cmp_e_21862
__cmp_t_28608:
	ld	hl, #1
__cmp_e_21862:
	dec	sp
	dec	sp
	ld	-6256(ix), l
	ld	-6255(ix), h
	jp	__xcc_L1333
__xcc_L1331:
	ld	hl, #1
	ld	-6256(ix), l
	ld	-6255(ix), h
__xcc_L1333:
	ld	l, -6256(ix)
	ld	h, -6255(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1328
	jp	__xcc_L1329
__xcc_L1328:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6258(ix), l
	ld	-6257(ix), h
	ld	l, -6258(ix)
	ld	h, -6257(ix)
	dec	sp
	dec	sp
	ld	-6260(ix), l
	ld	-6259(ix), h
	jp	__xcc_L1330
__xcc_L1329:
	ld	hl, #1
	ld	-6260(ix), l
	ld	-6259(ix), h
__xcc_L1330:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6262(ix), l
	ld	-6261(ix), h
	.globl __mul16
	ld	l, -6262(ix)
	ld	h, -6261(ix)
	push	hl
	ld	l, -6260(ix)
	ld	h, -6259(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6264(ix), l
	ld	-6263(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6266(ix), l
	ld	-6265(ix), h
	ld	l, -6266(ix)
	ld	h, -6265(ix)
	dec	sp
	dec	sp
	ld	-6268(ix), l
	ld	-6267(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6276(ix), l
	ld	-6275(ix), h
	ld	l, -6268(ix)
	ld	h, -6267(ix)
	push	hl
	ld	l, -6276(ix)
	ld	h, -6275(ix)
	ld	b, l
	pop	hl
__shift_1768:
	ld	a, b
	or	a, a
	jp	z, __sdone_9895
	add	hl, hl
	djnz	__shift_1768
__sdone_9895:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6284(ix), l
	ld	-6283(ix), h
	ld	l, -6284(ix)
	ld	h, -6283(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71198
	ld	hl, #0
	jp	__cmp_e_87660
__cmp_t_71198:
	ld	hl, #1
__cmp_e_87660:
	dec	sp
	dec	sp
	ld	-6286(ix), l
	ld	-6285(ix), h
	ld	l, -6286(ix)
	ld	h, -6285(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_54968
	ld	hl, #0
	jp	__cmp_e_22376
__cmp_t_54968:
	ld	hl, #1
__cmp_e_22376:
	dec	sp
	dec	sp
	ld	-6288(ix), l
	ld	-6287(ix), h
	ld	l, -6288(ix)
	ld	h, -6287(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1337
	jp	__xcc_L1338
__xcc_L1338:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6290(ix), l
	ld	-6289(ix), h
	ld	l, -6290(ix)
	ld	h, -6289(ix)
	dec	sp
	dec	sp
	ld	-6292(ix), l
	ld	-6291(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6300(ix), l
	ld	-6299(ix), h
	ld	l, -6292(ix)
	ld	h, -6291(ix)
	push	hl
	ld	l, -6300(ix)
	ld	h, -6299(ix)
	ld	b, l
	pop	hl
__shift_1009:
	ld	a, b
	or	a, a
	jp	z, __sdone_2173
	add	hl, hl
	djnz	__shift_1009
__sdone_2173:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6308(ix), l
	ld	-6307(ix), h
	ld	l, -6308(ix)
	ld	h, -6307(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6310(ix), l
	ld	-6309(ix), h
	ld	l, -6310(ix)
	ld	h, -6309(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89503
	ld	hl, #0
	jp	__cmp_e_74887
__cmp_t_89503:
	ld	hl, #1
__cmp_e_74887:
	dec	sp
	dec	sp
	ld	-6312(ix), l
	ld	-6311(ix), h
	ld	l, -6312(ix)
	ld	h, -6311(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71254
	ld	hl, #0
	jp	__cmp_e_54673
__cmp_t_71254:
	ld	hl, #1
__cmp_e_54673:
	dec	sp
	dec	sp
	ld	-6314(ix), l
	ld	-6313(ix), h
	jp	__xcc_L1339
__xcc_L1337:
	ld	hl, #1
	ld	-6314(ix), l
	ld	-6313(ix), h
__xcc_L1339:
	ld	l, -6314(ix)
	ld	h, -6313(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1334
	jp	__xcc_L1335
__xcc_L1334:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6316(ix), l
	ld	-6315(ix), h
	ld	l, -6316(ix)
	ld	h, -6315(ix)
	dec	sp
	dec	sp
	ld	-6318(ix), l
	ld	-6317(ix), h
	jp	__xcc_L1336
__xcc_L1335:
	ld	hl, #1
	ld	-6318(ix), l
	ld	-6317(ix), h
__xcc_L1336:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-6320(ix), l
	ld	-6319(ix), h
	.globl __mul16
	ld	l, -6320(ix)
	ld	h, -6319(ix)
	push	hl
	ld	l, -6318(ix)
	ld	h, -6317(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6322(ix), l
	ld	-6321(ix), h
	ld	l, -6322(ix)
	ld	h, -6321(ix)
	push	hl
	ld	l, -6264(ix)
	ld	h, -6263(ix)
	push	hl
	ld	l, -6238(ix)
	ld	h, -6237(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1312:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1311
	jp	__xcc_L1313
__xcc_L1313:
__xcc_L1222:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1221
	jp	__xcc_L1223
__xcc_L1223:
__xcc_L1340:
__xcc_L1343:
	ld	hl, #__str_1346
	dec	sp
	dec	sp
	ld	-6324(ix), l
	ld	-6323(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6326(ix), l
	ld	-6325(ix), h
	ld	l, -6326(ix)
	ld	h, -6325(ix)
	dec	sp
	dec	sp
	ld	-6328(ix), l
	ld	-6327(ix), h
	ld	l, -6328(ix)
	ld	h, -6327(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_94057
	ld	hl, #0
	jp	__cmp_e_18481
__cmp_t_94057:
	ld	hl, #1
__cmp_e_18481:
	dec	sp
	dec	sp
	ld	-6330(ix), l
	ld	-6329(ix), h
	ld	l, -6330(ix)
	ld	h, -6329(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43823
	ld	hl, #0
	jp	__cmp_e_21929
__cmp_t_43823:
	ld	hl, #1
__cmp_e_21929:
	dec	sp
	dec	sp
	ld	-6332(ix), l
	ld	-6331(ix), h
	ld	l, -6332(ix)
	ld	h, -6331(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1350
	jp	__xcc_L1351
__xcc_L1351:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6334(ix), l
	ld	-6333(ix), h
	ld	l, -6334(ix)
	ld	h, -6333(ix)
	dec	sp
	dec	sp
	ld	-6336(ix), l
	ld	-6335(ix), h
	ld	l, -6336(ix)
	ld	h, -6335(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6338(ix), l
	ld	-6337(ix), h
	ld	l, -6338(ix)
	ld	h, -6337(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74396
	ld	hl, #0
	jp	__cmp_e_83044
__cmp_t_74396:
	ld	hl, #1
__cmp_e_83044:
	dec	sp
	dec	sp
	ld	-6340(ix), l
	ld	-6339(ix), h
	ld	l, -6340(ix)
	ld	h, -6339(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_31942
	ld	hl, #0
	jp	__cmp_e_52280
__cmp_t_31942:
	ld	hl, #1
__cmp_e_52280:
	dec	sp
	dec	sp
	ld	-6342(ix), l
	ld	-6341(ix), h
	jp	__xcc_L1352
__xcc_L1350:
	ld	hl, #1
	ld	-6342(ix), l
	ld	-6341(ix), h
__xcc_L1352:
	ld	l, -6342(ix)
	ld	h, -6341(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1347
	jp	__xcc_L1348
__xcc_L1347:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6344(ix), l
	ld	-6343(ix), h
	ld	l, -6344(ix)
	ld	h, -6343(ix)
	dec	sp
	dec	sp
	ld	-6346(ix), l
	ld	-6345(ix), h
	jp	__xcc_L1349
__xcc_L1348:
	ld	hl, #1
	ld	-6346(ix), l
	ld	-6345(ix), h
__xcc_L1349:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6348(ix), l
	ld	-6347(ix), h
	.globl __mul16
	ld	l, -6348(ix)
	ld	h, -6347(ix)
	push	hl
	ld	l, -6346(ix)
	ld	h, -6345(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6350(ix), l
	ld	-6349(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6352(ix), l
	ld	-6351(ix), h
	ld	l, -6352(ix)
	ld	h, -6351(ix)
	dec	sp
	dec	sp
	ld	-6354(ix), l
	ld	-6353(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6356(ix), l
	ld	-6355(ix), h
	ld	l, -6354(ix)
	ld	h, -6353(ix)
	push	hl
	ld	l, -6356(ix)
	ld	h, -6355(ix)
	ld	b, l
	pop	hl
__shift_8012:
	ld	a, b
	or	a, a
	jp	z, __sdone_4209
	add	hl, hl
	djnz	__shift_8012
__sdone_4209:
	dec	sp
	dec	sp
	ld	-6358(ix), l
	ld	-6357(ix), h
	ld	l, -6358(ix)
	ld	h, -6357(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_96855
	ld	hl, #0
	jp	__cmp_e_39747
__cmp_t_96855:
	ld	hl, #1
__cmp_e_39747:
	dec	sp
	dec	sp
	ld	-6360(ix), l
	ld	-6359(ix), h
	ld	l, -6360(ix)
	ld	h, -6359(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44854
	ld	hl, #0
	jp	__cmp_e_52366
__cmp_t_44854:
	ld	hl, #1
__cmp_e_52366:
	dec	sp
	dec	sp
	ld	-6362(ix), l
	ld	-6361(ix), h
	ld	l, -6362(ix)
	ld	h, -6361(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1356
	jp	__xcc_L1357
__xcc_L1357:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6364(ix), l
	ld	-6363(ix), h
	ld	l, -6364(ix)
	ld	h, -6363(ix)
	dec	sp
	dec	sp
	ld	-6366(ix), l
	ld	-6365(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6368(ix), l
	ld	-6367(ix), h
	ld	l, -6366(ix)
	ld	h, -6365(ix)
	push	hl
	ld	l, -6368(ix)
	ld	h, -6367(ix)
	ld	b, l
	pop	hl
__shift_3134:
	ld	a, b
	or	a, a
	jp	z, __sdone_3759
	add	hl, hl
	djnz	__shift_3134
__sdone_3759:
	dec	sp
	dec	sp
	ld	-6370(ix), l
	ld	-6369(ix), h
	ld	l, -6370(ix)
	ld	h, -6369(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6372(ix), l
	ld	-6371(ix), h
	ld	l, -6372(ix)
	ld	h, -6371(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1281
	ld	hl, #0
	jp	__cmp_e_31742
__cmp_t_1281:
	ld	hl, #1
__cmp_e_31742:
	dec	sp
	dec	sp
	ld	-6374(ix), l
	ld	-6373(ix), h
	ld	l, -6374(ix)
	ld	h, -6373(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41973
	ld	hl, #0
	jp	__cmp_e_69401
__cmp_t_41973:
	ld	hl, #1
__cmp_e_69401:
	dec	sp
	dec	sp
	ld	-6376(ix), l
	ld	-6375(ix), h
	jp	__xcc_L1358
__xcc_L1356:
	ld	hl, #1
	ld	-6376(ix), l
	ld	-6375(ix), h
__xcc_L1358:
	ld	l, -6376(ix)
	ld	h, -6375(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1353
	jp	__xcc_L1354
__xcc_L1353:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6378(ix), l
	ld	-6377(ix), h
	ld	l, -6378(ix)
	ld	h, -6377(ix)
	dec	sp
	dec	sp
	ld	-6380(ix), l
	ld	-6379(ix), h
	jp	__xcc_L1355
__xcc_L1354:
	ld	hl, #1
	ld	-6380(ix), l
	ld	-6379(ix), h
__xcc_L1355:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6382(ix), l
	ld	-6381(ix), h
	.globl __mul16
	ld	l, -6382(ix)
	ld	h, -6381(ix)
	push	hl
	ld	l, -6380(ix)
	ld	h, -6379(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6384(ix), l
	ld	-6383(ix), h
	ld	l, -6384(ix)
	ld	h, -6383(ix)
	push	hl
	ld	l, -6350(ix)
	ld	h, -6349(ix)
	push	hl
	ld	l, -6324(ix)
	ld	h, -6323(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1359
	dec	sp
	dec	sp
	ld	-6386(ix), l
	ld	-6385(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6388(ix), l
	ld	-6387(ix), h
	ld	l, -6388(ix)
	ld	h, -6387(ix)
	dec	sp
	dec	sp
	ld	-6390(ix), l
	ld	-6389(ix), h
	ld	l, -6390(ix)
	ld	h, -6389(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_91638
	ld	hl, #0
	jp	__cmp_e_13171
__cmp_t_91638:
	ld	hl, #1
__cmp_e_13171:
	dec	sp
	dec	sp
	ld	-6392(ix), l
	ld	-6391(ix), h
	ld	l, -6392(ix)
	ld	h, -6391(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73413
	ld	hl, #0
	jp	__cmp_e_62958
__cmp_t_73413:
	ld	hl, #1
__cmp_e_62958:
	dec	sp
	dec	sp
	ld	-6394(ix), l
	ld	-6393(ix), h
	ld	l, -6394(ix)
	ld	h, -6393(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1363
	jp	__xcc_L1364
__xcc_L1364:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6396(ix), l
	ld	-6395(ix), h
	ld	l, -6396(ix)
	ld	h, -6395(ix)
	dec	sp
	dec	sp
	ld	-6398(ix), l
	ld	-6397(ix), h
	ld	l, -6398(ix)
	ld	h, -6397(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6400(ix), l
	ld	-6399(ix), h
	ld	l, -6400(ix)
	ld	h, -6399(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51899
	ld	hl, #0
	jp	__cmp_e_14422
__cmp_t_51899:
	ld	hl, #1
__cmp_e_14422:
	dec	sp
	dec	sp
	ld	-6402(ix), l
	ld	-6401(ix), h
	ld	l, -6402(ix)
	ld	h, -6401(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41484
	ld	hl, #0
	jp	__cmp_e_57755
__cmp_t_41484:
	ld	hl, #1
__cmp_e_57755:
	dec	sp
	dec	sp
	ld	-6404(ix), l
	ld	-6403(ix), h
	jp	__xcc_L1365
__xcc_L1363:
	ld	hl, #1
	ld	-6404(ix), l
	ld	-6403(ix), h
__xcc_L1365:
	ld	l, -6404(ix)
	ld	h, -6403(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1360
	jp	__xcc_L1361
__xcc_L1360:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6406(ix), l
	ld	-6405(ix), h
	ld	l, -6406(ix)
	ld	h, -6405(ix)
	dec	sp
	dec	sp
	ld	-6408(ix), l
	ld	-6407(ix), h
	jp	__xcc_L1362
__xcc_L1361:
	ld	hl, #1
	ld	-6408(ix), l
	ld	-6407(ix), h
__xcc_L1362:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6410(ix), l
	ld	-6409(ix), h
	.globl __mul16
	ld	l, -6410(ix)
	ld	h, -6409(ix)
	push	hl
	ld	l, -6408(ix)
	ld	h, -6407(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6412(ix), l
	ld	-6411(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6414(ix), l
	ld	-6413(ix), h
	ld	l, -6414(ix)
	ld	h, -6413(ix)
	dec	sp
	dec	sp
	ld	-6416(ix), l
	ld	-6415(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6418(ix), l
	ld	-6417(ix), h
	ld	l, -6416(ix)
	ld	h, -6415(ix)
	push	hl
	ld	l, -6418(ix)
	ld	h, -6417(ix)
	ld	b, l
	pop	hl
__shift_5661:
	ld	a, b
	or	a, a
	jp	z, __sdone_9090
	add	hl, hl
	djnz	__shift_5661
__sdone_9090:
	dec	sp
	dec	sp
	ld	-6420(ix), l
	ld	-6419(ix), h
	ld	l, -6420(ix)
	ld	h, -6419(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28780
	ld	hl, #0
	jp	__cmp_e_16071
__cmp_t_28780:
	ld	hl, #1
__cmp_e_16071:
	dec	sp
	dec	sp
	ld	-6422(ix), l
	ld	-6421(ix), h
	ld	l, -6422(ix)
	ld	h, -6421(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_63923
	ld	hl, #0
	jp	__cmp_e_72603
__cmp_t_63923:
	ld	hl, #1
__cmp_e_72603:
	dec	sp
	dec	sp
	ld	-6424(ix), l
	ld	-6423(ix), h
	ld	l, -6424(ix)
	ld	h, -6423(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1369
	jp	__xcc_L1370
__xcc_L1370:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6426(ix), l
	ld	-6425(ix), h
	ld	l, -6426(ix)
	ld	h, -6425(ix)
	dec	sp
	dec	sp
	ld	-6428(ix), l
	ld	-6427(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6430(ix), l
	ld	-6429(ix), h
	ld	l, -6428(ix)
	ld	h, -6427(ix)
	push	hl
	ld	l, -6430(ix)
	ld	h, -6429(ix)
	ld	b, l
	pop	hl
__shift_8000:
	ld	a, b
	or	a, a
	jp	z, __sdone_8320
	add	hl, hl
	djnz	__shift_8000
__sdone_8320:
	dec	sp
	dec	sp
	ld	-6432(ix), l
	ld	-6431(ix), h
	ld	l, -6432(ix)
	ld	h, -6431(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6434(ix), l
	ld	-6433(ix), h
	ld	l, -6434(ix)
	ld	h, -6433(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72000
	ld	hl, #0
	jp	__cmp_e_69942
__cmp_t_72000:
	ld	hl, #1
__cmp_e_69942:
	dec	sp
	dec	sp
	ld	-6436(ix), l
	ld	-6435(ix), h
	ld	l, -6436(ix)
	ld	h, -6435(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_6952
	ld	hl, #0
	jp	__cmp_e_40012
__cmp_t_6952:
	ld	hl, #1
__cmp_e_40012:
	dec	sp
	dec	sp
	ld	-6438(ix), l
	ld	-6437(ix), h
	jp	__xcc_L1371
__xcc_L1369:
	ld	hl, #1
	ld	-6438(ix), l
	ld	-6437(ix), h
__xcc_L1371:
	ld	l, -6438(ix)
	ld	h, -6437(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1366
	jp	__xcc_L1367
__xcc_L1366:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6440(ix), l
	ld	-6439(ix), h
	ld	l, -6440(ix)
	ld	h, -6439(ix)
	dec	sp
	dec	sp
	ld	-6442(ix), l
	ld	-6441(ix), h
	jp	__xcc_L1368
__xcc_L1367:
	ld	hl, #1
	ld	-6442(ix), l
	ld	-6441(ix), h
__xcc_L1368:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6444(ix), l
	ld	-6443(ix), h
	.globl __mul16
	ld	l, -6444(ix)
	ld	h, -6443(ix)
	push	hl
	ld	l, -6442(ix)
	ld	h, -6441(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6446(ix), l
	ld	-6445(ix), h
	ld	l, -6446(ix)
	ld	h, -6445(ix)
	push	hl
	ld	l, -6412(ix)
	ld	h, -6411(ix)
	push	hl
	ld	l, -6386(ix)
	ld	h, -6385(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1344:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1343
	jp	__xcc_L1345
__xcc_L1345:
__xcc_L1372:
	ld	hl, #__str_1375
	dec	sp
	dec	sp
	ld	-6448(ix), l
	ld	-6447(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6450(ix), l
	ld	-6449(ix), h
	ld	l, -6450(ix)
	ld	h, -6449(ix)
	dec	sp
	dec	sp
	ld	-6452(ix), l
	ld	-6451(ix), h
	ld	l, -6452(ix)
	ld	h, -6451(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30504
	ld	hl, #0
	jp	__cmp_e_3807
__cmp_t_30504:
	ld	hl, #1
__cmp_e_3807:
	dec	sp
	dec	sp
	ld	-6454(ix), l
	ld	-6453(ix), h
	ld	l, -6454(ix)
	ld	h, -6453(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79759
	ld	hl, #0
	jp	__cmp_e_75358
__cmp_t_79759:
	ld	hl, #1
__cmp_e_75358:
	dec	sp
	dec	sp
	ld	-6456(ix), l
	ld	-6455(ix), h
	ld	l, -6456(ix)
	ld	h, -6455(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1379
	jp	__xcc_L1380
__xcc_L1380:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6458(ix), l
	ld	-6457(ix), h
	ld	l, -6458(ix)
	ld	h, -6457(ix)
	dec	sp
	dec	sp
	ld	-6460(ix), l
	ld	-6459(ix), h
	ld	l, -6460(ix)
	ld	h, -6459(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6462(ix), l
	ld	-6461(ix), h
	ld	l, -6462(ix)
	ld	h, -6461(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72525
	ld	hl, #0
	jp	__cmp_e_82894
__cmp_t_72525:
	ld	hl, #1
__cmp_e_82894:
	dec	sp
	dec	sp
	ld	-6464(ix), l
	ld	-6463(ix), h
	ld	l, -6464(ix)
	ld	h, -6463(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79117
	ld	hl, #0
	jp	__cmp_e_90158
__cmp_t_79117:
	ld	hl, #1
__cmp_e_90158:
	dec	sp
	dec	sp
	ld	-6466(ix), l
	ld	-6465(ix), h
	jp	__xcc_L1381
__xcc_L1379:
	ld	hl, #1
	ld	-6466(ix), l
	ld	-6465(ix), h
__xcc_L1381:
	ld	l, -6466(ix)
	ld	h, -6465(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1376
	jp	__xcc_L1377
__xcc_L1376:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6468(ix), l
	ld	-6467(ix), h
	ld	l, -6468(ix)
	ld	h, -6467(ix)
	dec	sp
	dec	sp
	ld	-6470(ix), l
	ld	-6469(ix), h
	jp	__xcc_L1378
__xcc_L1377:
	ld	hl, #1
	ld	-6470(ix), l
	ld	-6469(ix), h
__xcc_L1378:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6472(ix), l
	ld	-6471(ix), h
	.globl __mul16
	ld	l, -6472(ix)
	ld	h, -6471(ix)
	push	hl
	ld	l, -6470(ix)
	ld	h, -6469(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6474(ix), l
	ld	-6473(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6476(ix), l
	ld	-6475(ix), h
	ld	l, -6476(ix)
	ld	h, -6475(ix)
	dec	sp
	dec	sp
	ld	-6478(ix), l
	ld	-6477(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6480(ix), l
	ld	-6479(ix), h
	ld	l, -6478(ix)
	ld	h, -6477(ix)
	push	hl
	ld	l, -6480(ix)
	ld	h, -6479(ix)
	ld	b, l
	pop	hl
__shift_4636:
	ld	a, b
	or	a, a
	jp	z, __sdone_1090
	add	hl, hl
	djnz	__shift_4636
__sdone_1090:
	dec	sp
	dec	sp
	ld	-6482(ix), l
	ld	-6481(ix), h
	ld	l, -6482(ix)
	ld	h, -6481(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59560
	ld	hl, #0
	jp	__cmp_e_22626
__cmp_t_59560:
	ld	hl, #1
__cmp_e_22626:
	dec	sp
	dec	sp
	ld	-6484(ix), l
	ld	-6483(ix), h
	ld	l, -6484(ix)
	ld	h, -6483(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50614
	ld	hl, #0
	jp	__cmp_e_32973
__cmp_t_50614:
	ld	hl, #1
__cmp_e_32973:
	dec	sp
	dec	sp
	ld	-6486(ix), l
	ld	-6485(ix), h
	ld	l, -6486(ix)
	ld	h, -6485(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1385
	jp	__xcc_L1386
__xcc_L1386:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6488(ix), l
	ld	-6487(ix), h
	ld	l, -6488(ix)
	ld	h, -6487(ix)
	dec	sp
	dec	sp
	ld	-6490(ix), l
	ld	-6489(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6492(ix), l
	ld	-6491(ix), h
	ld	l, -6490(ix)
	ld	h, -6489(ix)
	push	hl
	ld	l, -6492(ix)
	ld	h, -6491(ix)
	ld	b, l
	pop	hl
__shift_1937:
	ld	a, b
	or	a, a
	jp	z, __sdone_8865
	add	hl, hl
	djnz	__shift_1937
__sdone_8865:
	dec	sp
	dec	sp
	ld	-6494(ix), l
	ld	-6493(ix), h
	ld	l, -6494(ix)
	ld	h, -6493(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6496(ix), l
	ld	-6495(ix), h
	ld	l, -6496(ix)
	ld	h, -6495(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63748
	ld	hl, #0
	jp	__cmp_e_43421
__cmp_t_63748:
	ld	hl, #1
__cmp_e_43421:
	dec	sp
	dec	sp
	ld	-6498(ix), l
	ld	-6497(ix), h
	ld	l, -6498(ix)
	ld	h, -6497(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76620
	ld	hl, #0
	jp	__cmp_e_69409
__cmp_t_76620:
	ld	hl, #1
__cmp_e_69409:
	dec	sp
	dec	sp
	ld	-6500(ix), l
	ld	-6499(ix), h
	jp	__xcc_L1387
__xcc_L1385:
	ld	hl, #1
	ld	-6500(ix), l
	ld	-6499(ix), h
__xcc_L1387:
	ld	l, -6500(ix)
	ld	h, -6499(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1382
	jp	__xcc_L1383
__xcc_L1382:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6502(ix), l
	ld	-6501(ix), h
	ld	l, -6502(ix)
	ld	h, -6501(ix)
	dec	sp
	dec	sp
	ld	-6504(ix), l
	ld	-6503(ix), h
	jp	__xcc_L1384
__xcc_L1383:
	ld	hl, #1
	ld	-6504(ix), l
	ld	-6503(ix), h
__xcc_L1384:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6506(ix), l
	ld	-6505(ix), h
	.globl __mul16
	ld	l, -6506(ix)
	ld	h, -6505(ix)
	push	hl
	ld	l, -6504(ix)
	ld	h, -6503(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6508(ix), l
	ld	-6507(ix), h
	ld	l, -6508(ix)
	ld	h, -6507(ix)
	push	hl
	ld	l, -6474(ix)
	ld	h, -6473(ix)
	push	hl
	ld	l, -6448(ix)
	ld	h, -6447(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1388
	dec	sp
	dec	sp
	ld	-6510(ix), l
	ld	-6509(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6512(ix), l
	ld	-6511(ix), h
	ld	l, -6512(ix)
	ld	h, -6511(ix)
	dec	sp
	dec	sp
	ld	-6514(ix), l
	ld	-6513(ix), h
	ld	l, -6514(ix)
	ld	h, -6513(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_88863
	ld	hl, #0
	jp	__cmp_e_5400
__cmp_t_88863:
	ld	hl, #1
__cmp_e_5400:
	dec	sp
	dec	sp
	ld	-6516(ix), l
	ld	-6515(ix), h
	ld	l, -6516(ix)
	ld	h, -6515(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_1832
	ld	hl, #0
	jp	__cmp_e_52786
__cmp_t_1832:
	ld	hl, #1
__cmp_e_52786:
	dec	sp
	dec	sp
	ld	-6518(ix), l
	ld	-6517(ix), h
	ld	l, -6518(ix)
	ld	h, -6517(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1392
	jp	__xcc_L1393
__xcc_L1393:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6520(ix), l
	ld	-6519(ix), h
	ld	l, -6520(ix)
	ld	h, -6519(ix)
	dec	sp
	dec	sp
	ld	-6522(ix), l
	ld	-6521(ix), h
	ld	l, -6522(ix)
	ld	h, -6521(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6524(ix), l
	ld	-6523(ix), h
	ld	l, -6524(ix)
	ld	h, -6523(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78004
	ld	hl, #0
	jp	__cmp_e_39833
__cmp_t_78004:
	ld	hl, #1
__cmp_e_39833:
	dec	sp
	dec	sp
	ld	-6526(ix), l
	ld	-6525(ix), h
	ld	l, -6526(ix)
	ld	h, -6525(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_7458
	ld	hl, #0
	jp	__cmp_e_66356
__cmp_t_7458:
	ld	hl, #1
__cmp_e_66356:
	dec	sp
	dec	sp
	ld	-6528(ix), l
	ld	-6527(ix), h
	jp	__xcc_L1394
__xcc_L1392:
	ld	hl, #1
	ld	-6528(ix), l
	ld	-6527(ix), h
__xcc_L1394:
	ld	l, -6528(ix)
	ld	h, -6527(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1389
	jp	__xcc_L1390
__xcc_L1389:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6530(ix), l
	ld	-6529(ix), h
	ld	l, -6530(ix)
	ld	h, -6529(ix)
	dec	sp
	dec	sp
	ld	-6532(ix), l
	ld	-6531(ix), h
	jp	__xcc_L1391
__xcc_L1390:
	ld	hl, #1
	ld	-6532(ix), l
	ld	-6531(ix), h
__xcc_L1391:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6534(ix), l
	ld	-6533(ix), h
	.globl __mul16
	ld	l, -6534(ix)
	ld	h, -6533(ix)
	push	hl
	ld	l, -6532(ix)
	ld	h, -6531(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6536(ix), l
	ld	-6535(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6538(ix), l
	ld	-6537(ix), h
	ld	l, -6538(ix)
	ld	h, -6537(ix)
	dec	sp
	dec	sp
	ld	-6540(ix), l
	ld	-6539(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6542(ix), l
	ld	-6541(ix), h
	ld	l, -6540(ix)
	ld	h, -6539(ix)
	push	hl
	ld	l, -6542(ix)
	ld	h, -6541(ix)
	ld	b, l
	pop	hl
__shift_6127:
	ld	a, b
	or	a, a
	jp	z, __sdone_4410
	add	hl, hl
	djnz	__shift_6127
__sdone_4410:
	dec	sp
	dec	sp
	ld	-6544(ix), l
	ld	-6543(ix), h
	ld	l, -6544(ix)
	ld	h, -6543(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_6368
	ld	hl, #0
	jp	__cmp_e_56631
__cmp_t_6368:
	ld	hl, #1
__cmp_e_56631:
	dec	sp
	dec	sp
	ld	-6546(ix), l
	ld	-6545(ix), h
	ld	l, -6546(ix)
	ld	h, -6545(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34569
	ld	hl, #0
	jp	__cmp_e_86128
__cmp_t_34569:
	ld	hl, #1
__cmp_e_86128:
	dec	sp
	dec	sp
	ld	-6548(ix), l
	ld	-6547(ix), h
	ld	l, -6548(ix)
	ld	h, -6547(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1398
	jp	__xcc_L1399
__xcc_L1399:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6550(ix), l
	ld	-6549(ix), h
	ld	l, -6550(ix)
	ld	h, -6549(ix)
	dec	sp
	dec	sp
	ld	-6552(ix), l
	ld	-6551(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6554(ix), l
	ld	-6553(ix), h
	ld	l, -6552(ix)
	ld	h, -6551(ix)
	push	hl
	ld	l, -6554(ix)
	ld	h, -6553(ix)
	ld	b, l
	pop	hl
__shift_8341:
	ld	a, b
	or	a, a
	jp	z, __sdone_3446
	add	hl, hl
	djnz	__shift_8341
__sdone_3446:
	dec	sp
	dec	sp
	ld	-6556(ix), l
	ld	-6555(ix), h
	ld	l, -6556(ix)
	ld	h, -6555(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6558(ix), l
	ld	-6557(ix), h
	ld	l, -6558(ix)
	ld	h, -6557(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85374
	ld	hl, #0
	jp	__cmp_e_27458
__cmp_t_85374:
	ld	hl, #1
__cmp_e_27458:
	dec	sp
	dec	sp
	ld	-6560(ix), l
	ld	-6559(ix), h
	ld	l, -6560(ix)
	ld	h, -6559(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13605
	ld	hl, #0
	jp	__cmp_e_10
__cmp_t_13605:
	ld	hl, #1
__cmp_e_10:
	dec	sp
	dec	sp
	ld	-6562(ix), l
	ld	-6561(ix), h
	jp	__xcc_L1400
__xcc_L1398:
	ld	hl, #1
	ld	-6562(ix), l
	ld	-6561(ix), h
__xcc_L1400:
	ld	l, -6562(ix)
	ld	h, -6561(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1395
	jp	__xcc_L1396
__xcc_L1395:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6564(ix), l
	ld	-6563(ix), h
	ld	l, -6564(ix)
	ld	h, -6563(ix)
	dec	sp
	dec	sp
	ld	-6566(ix), l
	ld	-6565(ix), h
	jp	__xcc_L1397
__xcc_L1396:
	ld	hl, #1
	ld	-6566(ix), l
	ld	-6565(ix), h
__xcc_L1397:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6568(ix), l
	ld	-6567(ix), h
	.globl __mul16
	ld	l, -6568(ix)
	ld	h, -6567(ix)
	push	hl
	ld	l, -6566(ix)
	ld	h, -6565(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6570(ix), l
	ld	-6569(ix), h
	ld	l, -6570(ix)
	ld	h, -6569(ix)
	push	hl
	ld	l, -6536(ix)
	ld	h, -6535(ix)
	push	hl
	ld	l, -6510(ix)
	ld	h, -6509(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1373:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1372
	jp	__xcc_L1374
__xcc_L1374:
__xcc_L1401:
	ld	hl, #__str_1404
	dec	sp
	dec	sp
	ld	-6572(ix), l
	ld	-6571(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6574(ix), l
	ld	-6573(ix), h
	ld	l, -6574(ix)
	ld	h, -6573(ix)
	dec	sp
	dec	sp
	ld	-6576(ix), l
	ld	-6575(ix), h
	ld	l, -6576(ix)
	ld	h, -6575(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_64901
	ld	hl, #0
	jp	__cmp_e_73165
__cmp_t_64901:
	ld	hl, #1
__cmp_e_73165:
	dec	sp
	dec	sp
	ld	-6578(ix), l
	ld	-6577(ix), h
	ld	l, -6578(ix)
	ld	h, -6577(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_38989
	ld	hl, #0
	jp	__cmp_e_31867
__cmp_t_38989:
	ld	hl, #1
__cmp_e_31867:
	dec	sp
	dec	sp
	ld	-6580(ix), l
	ld	-6579(ix), h
	ld	l, -6580(ix)
	ld	h, -6579(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1408
	jp	__xcc_L1409
__xcc_L1409:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6582(ix), l
	ld	-6581(ix), h
	ld	l, -6582(ix)
	ld	h, -6581(ix)
	dec	sp
	dec	sp
	ld	-6584(ix), l
	ld	-6583(ix), h
	ld	l, -6584(ix)
	ld	h, -6583(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6586(ix), l
	ld	-6585(ix), h
	ld	l, -6586(ix)
	ld	h, -6585(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_22490
	ld	hl, #0
	jp	__cmp_e_40926
__cmp_t_22490:
	ld	hl, #1
__cmp_e_40926:
	dec	sp
	dec	sp
	ld	-6588(ix), l
	ld	-6587(ix), h
	ld	l, -6588(ix)
	ld	h, -6587(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50732
	ld	hl, #0
	jp	__cmp_e_86238
__cmp_t_50732:
	ld	hl, #1
__cmp_e_86238:
	dec	sp
	dec	sp
	ld	-6590(ix), l
	ld	-6589(ix), h
	jp	__xcc_L1410
__xcc_L1408:
	ld	hl, #1
	ld	-6590(ix), l
	ld	-6589(ix), h
__xcc_L1410:
	ld	l, -6590(ix)
	ld	h, -6589(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1405
	jp	__xcc_L1406
__xcc_L1405:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6592(ix), l
	ld	-6591(ix), h
	ld	l, -6592(ix)
	ld	h, -6591(ix)
	dec	sp
	dec	sp
	ld	-6594(ix), l
	ld	-6593(ix), h
	jp	__xcc_L1407
__xcc_L1406:
	ld	hl, #1
	ld	-6594(ix), l
	ld	-6593(ix), h
__xcc_L1407:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6596(ix), l
	ld	-6595(ix), h
	.globl __mul16
	ld	l, -6596(ix)
	ld	h, -6595(ix)
	push	hl
	ld	l, -6594(ix)
	ld	h, -6593(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6598(ix), l
	ld	-6597(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6600(ix), l
	ld	-6599(ix), h
	ld	l, -6600(ix)
	ld	h, -6599(ix)
	dec	sp
	dec	sp
	ld	-6602(ix), l
	ld	-6601(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6606(ix), l
	ld	-6605(ix), h
	ld	l, -6602(ix)
	ld	h, -6601(ix)
	push	hl
	ld	l, -6606(ix)
	ld	h, -6605(ix)
	ld	b, l
	pop	hl
__shift_699:
	ld	a, b
	or	a, a
	jp	z, __sdone_3705
	add	hl, hl
	djnz	__shift_699
__sdone_3705:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6610(ix), l
	ld	-6609(ix), h
	ld	l, -6610(ix)
	ld	h, -6609(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72000
	ld	hl, #0
	jp	__cmp_e_89562
__cmp_t_72000:
	ld	hl, #1
__cmp_e_89562:
	dec	sp
	dec	sp
	ld	-6612(ix), l
	ld	-6611(ix), h
	ld	l, -6612(ix)
	ld	h, -6611(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65457
	ld	hl, #0
	jp	__cmp_e_73832
__cmp_t_65457:
	ld	hl, #1
__cmp_e_73832:
	dec	sp
	dec	sp
	ld	-6614(ix), l
	ld	-6613(ix), h
	ld	l, -6614(ix)
	ld	h, -6613(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1414
	jp	__xcc_L1415
__xcc_L1415:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6616(ix), l
	ld	-6615(ix), h
	ld	l, -6616(ix)
	ld	h, -6615(ix)
	dec	sp
	dec	sp
	ld	-6618(ix), l
	ld	-6617(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6622(ix), l
	ld	-6621(ix), h
	ld	l, -6618(ix)
	ld	h, -6617(ix)
	push	hl
	ld	l, -6622(ix)
	ld	h, -6621(ix)
	ld	b, l
	pop	hl
__shift_2348:
	ld	a, b
	or	a, a
	jp	z, __sdone_3461
	add	hl, hl
	djnz	__shift_2348
__sdone_3461:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6626(ix), l
	ld	-6625(ix), h
	ld	l, -6626(ix)
	ld	h, -6625(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6628(ix), l
	ld	-6627(ix), h
	ld	l, -6628(ix)
	ld	h, -6627(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30017
	ld	hl, #0
	jp	__cmp_e_49807
__cmp_t_30017:
	ld	hl, #1
__cmp_e_49807:
	dec	sp
	dec	sp
	ld	-6630(ix), l
	ld	-6629(ix), h
	ld	l, -6630(ix)
	ld	h, -6629(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26169
	ld	hl, #0
	jp	__cmp_e_72497
__cmp_t_26169:
	ld	hl, #1
__cmp_e_72497:
	dec	sp
	dec	sp
	ld	-6632(ix), l
	ld	-6631(ix), h
	jp	__xcc_L1416
__xcc_L1414:
	ld	hl, #1
	ld	-6632(ix), l
	ld	-6631(ix), h
__xcc_L1416:
	ld	l, -6632(ix)
	ld	h, -6631(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1411
	jp	__xcc_L1412
__xcc_L1411:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6634(ix), l
	ld	-6633(ix), h
	ld	l, -6634(ix)
	ld	h, -6633(ix)
	dec	sp
	dec	sp
	ld	-6636(ix), l
	ld	-6635(ix), h
	jp	__xcc_L1413
__xcc_L1412:
	ld	hl, #1
	ld	-6636(ix), l
	ld	-6635(ix), h
__xcc_L1413:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6638(ix), l
	ld	-6637(ix), h
	.globl __mul16
	ld	l, -6638(ix)
	ld	h, -6637(ix)
	push	hl
	ld	l, -6636(ix)
	ld	h, -6635(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6640(ix), l
	ld	-6639(ix), h
	ld	l, -6640(ix)
	ld	h, -6639(ix)
	push	hl
	ld	l, -6598(ix)
	ld	h, -6597(ix)
	push	hl
	ld	l, -6572(ix)
	ld	h, -6571(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1417
	dec	sp
	dec	sp
	ld	-6642(ix), l
	ld	-6641(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6644(ix), l
	ld	-6643(ix), h
	ld	l, -6644(ix)
	ld	h, -6643(ix)
	dec	sp
	dec	sp
	ld	-6646(ix), l
	ld	-6645(ix), h
	ld	l, -6646(ix)
	ld	h, -6645(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80569
	ld	hl, #0
	jp	__cmp_e_32538
__cmp_t_80569:
	ld	hl, #1
__cmp_e_32538:
	dec	sp
	dec	sp
	ld	-6648(ix), l
	ld	-6647(ix), h
	ld	l, -6648(ix)
	ld	h, -6647(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_29128
	ld	hl, #0
	jp	__cmp_e_15139
__cmp_t_29128:
	ld	hl, #1
__cmp_e_15139:
	dec	sp
	dec	sp
	ld	-6650(ix), l
	ld	-6649(ix), h
	ld	l, -6650(ix)
	ld	h, -6649(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1421
	jp	__xcc_L1422
__xcc_L1422:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6652(ix), l
	ld	-6651(ix), h
	ld	l, -6652(ix)
	ld	h, -6651(ix)
	dec	sp
	dec	sp
	ld	-6654(ix), l
	ld	-6653(ix), h
	ld	l, -6654(ix)
	ld	h, -6653(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6656(ix), l
	ld	-6655(ix), h
	ld	l, -6656(ix)
	ld	h, -6655(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35018
	ld	hl, #0
	jp	__cmp_e_77470
__cmp_t_35018:
	ld	hl, #1
__cmp_e_77470:
	dec	sp
	dec	sp
	ld	-6658(ix), l
	ld	-6657(ix), h
	ld	l, -6658(ix)
	ld	h, -6657(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_38585
	ld	hl, #0
	jp	__cmp_e_20392
__cmp_t_38585:
	ld	hl, #1
__cmp_e_20392:
	dec	sp
	dec	sp
	ld	-6660(ix), l
	ld	-6659(ix), h
	jp	__xcc_L1423
__xcc_L1421:
	ld	hl, #1
	ld	-6660(ix), l
	ld	-6659(ix), h
__xcc_L1423:
	ld	l, -6660(ix)
	ld	h, -6659(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1418
	jp	__xcc_L1419
__xcc_L1418:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6662(ix), l
	ld	-6661(ix), h
	ld	l, -6662(ix)
	ld	h, -6661(ix)
	dec	sp
	dec	sp
	ld	-6664(ix), l
	ld	-6663(ix), h
	jp	__xcc_L1420
__xcc_L1419:
	ld	hl, #1
	ld	-6664(ix), l
	ld	-6663(ix), h
__xcc_L1420:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6666(ix), l
	ld	-6665(ix), h
	.globl __mul16
	ld	l, -6666(ix)
	ld	h, -6665(ix)
	push	hl
	ld	l, -6664(ix)
	ld	h, -6663(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6668(ix), l
	ld	-6667(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6670(ix), l
	ld	-6669(ix), h
	ld	l, -6670(ix)
	ld	h, -6669(ix)
	dec	sp
	dec	sp
	ld	-6672(ix), l
	ld	-6671(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6676(ix), l
	ld	-6675(ix), h
	ld	l, -6672(ix)
	ld	h, -6671(ix)
	push	hl
	ld	l, -6676(ix)
	ld	h, -6675(ix)
	ld	b, l
	pop	hl
__shift_1280:
	ld	a, b
	or	a, a
	jp	z, __sdone_8542
	add	hl, hl
	djnz	__shift_1280
__sdone_8542:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6680(ix), l
	ld	-6679(ix), h
	ld	l, -6680(ix)
	ld	h, -6679(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_36754
	ld	hl, #0
	jp	__cmp_e_2533
__cmp_t_36754:
	ld	hl, #1
__cmp_e_2533:
	dec	sp
	dec	sp
	ld	-6682(ix), l
	ld	-6681(ix), h
	ld	l, -6682(ix)
	ld	h, -6681(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41707
	ld	hl, #0
	jp	__cmp_e_75743
__cmp_t_41707:
	ld	hl, #1
__cmp_e_75743:
	dec	sp
	dec	sp
	ld	-6684(ix), l
	ld	-6683(ix), h
	ld	l, -6684(ix)
	ld	h, -6683(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1427
	jp	__xcc_L1428
__xcc_L1428:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6686(ix), l
	ld	-6685(ix), h
	ld	l, -6686(ix)
	ld	h, -6685(ix)
	dec	sp
	dec	sp
	ld	-6688(ix), l
	ld	-6687(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6692(ix), l
	ld	-6691(ix), h
	ld	l, -6688(ix)
	ld	h, -6687(ix)
	push	hl
	ld	l, -6692(ix)
	ld	h, -6691(ix)
	ld	b, l
	pop	hl
__shift_4400:
	ld	a, b
	or	a, a
	jp	z, __sdone_550
	add	hl, hl
	djnz	__shift_4400
__sdone_550:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6696(ix), l
	ld	-6695(ix), h
	ld	l, -6696(ix)
	ld	h, -6695(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6698(ix), l
	ld	-6697(ix), h
	ld	l, -6698(ix)
	ld	h, -6697(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_33021
	ld	hl, #0
	jp	__cmp_e_1485
__cmp_t_33021:
	ld	hl, #1
__cmp_e_1485:
	dec	sp
	dec	sp
	ld	-6700(ix), l
	ld	-6699(ix), h
	ld	l, -6700(ix)
	ld	h, -6699(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_66788
	ld	hl, #0
	jp	__cmp_e_33720
__cmp_t_66788:
	ld	hl, #1
__cmp_e_33720:
	dec	sp
	dec	sp
	ld	-6702(ix), l
	ld	-6701(ix), h
	jp	__xcc_L1429
__xcc_L1427:
	ld	hl, #1
	ld	-6702(ix), l
	ld	-6701(ix), h
__xcc_L1429:
	ld	l, -6702(ix)
	ld	h, -6701(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1424
	jp	__xcc_L1425
__xcc_L1424:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6704(ix), l
	ld	-6703(ix), h
	ld	l, -6704(ix)
	ld	h, -6703(ix)
	dec	sp
	dec	sp
	ld	-6706(ix), l
	ld	-6705(ix), h
	jp	__xcc_L1426
__xcc_L1425:
	ld	hl, #1
	ld	-6706(ix), l
	ld	-6705(ix), h
__xcc_L1426:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6708(ix), l
	ld	-6707(ix), h
	.globl __mul16
	ld	l, -6708(ix)
	ld	h, -6707(ix)
	push	hl
	ld	l, -6706(ix)
	ld	h, -6705(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6710(ix), l
	ld	-6709(ix), h
	ld	l, -6710(ix)
	ld	h, -6709(ix)
	push	hl
	ld	l, -6668(ix)
	ld	h, -6667(ix)
	push	hl
	ld	l, -6642(ix)
	ld	h, -6641(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1402:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1401
	jp	__xcc_L1403
__xcc_L1403:
__xcc_L1430:
	ld	hl, #__str_1433
	dec	sp
	dec	sp
	ld	-6712(ix), l
	ld	-6711(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6714(ix), l
	ld	-6713(ix), h
	ld	l, -6714(ix)
	ld	h, -6713(ix)
	dec	sp
	dec	sp
	ld	-6716(ix), l
	ld	-6715(ix), h
	ld	l, -6716(ix)
	ld	h, -6715(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_45190
	ld	hl, #0
	jp	__cmp_e_55140
__cmp_t_45190:
	ld	hl, #1
__cmp_e_55140:
	dec	sp
	dec	sp
	ld	-6718(ix), l
	ld	-6717(ix), h
	ld	l, -6718(ix)
	ld	h, -6717(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_39634
	ld	hl, #0
	jp	__cmp_e_10647
__cmp_t_39634:
	ld	hl, #1
__cmp_e_10647:
	dec	sp
	dec	sp
	ld	-6720(ix), l
	ld	-6719(ix), h
	ld	l, -6720(ix)
	ld	h, -6719(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1437
	jp	__xcc_L1438
__xcc_L1438:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6722(ix), l
	ld	-6721(ix), h
	ld	l, -6722(ix)
	ld	h, -6721(ix)
	dec	sp
	dec	sp
	ld	-6724(ix), l
	ld	-6723(ix), h
	ld	l, -6724(ix)
	ld	h, -6723(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6726(ix), l
	ld	-6725(ix), h
	ld	l, -6726(ix)
	ld	h, -6725(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_45325
	ld	hl, #0
	jp	__cmp_e_81983
__cmp_t_45325:
	ld	hl, #1
__cmp_e_81983:
	dec	sp
	dec	sp
	ld	-6728(ix), l
	ld	-6727(ix), h
	ld	l, -6728(ix)
	ld	h, -6727(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70461
	ld	hl, #0
	jp	__cmp_e_91694
__cmp_t_70461:
	ld	hl, #1
__cmp_e_91694:
	dec	sp
	dec	sp
	ld	-6730(ix), l
	ld	-6729(ix), h
	jp	__xcc_L1439
__xcc_L1437:
	ld	hl, #1
	ld	-6730(ix), l
	ld	-6729(ix), h
__xcc_L1439:
	ld	l, -6730(ix)
	ld	h, -6729(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1434
	jp	__xcc_L1435
__xcc_L1434:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6732(ix), l
	ld	-6731(ix), h
	ld	l, -6732(ix)
	ld	h, -6731(ix)
	dec	sp
	dec	sp
	ld	-6734(ix), l
	ld	-6733(ix), h
	jp	__xcc_L1436
__xcc_L1435:
	ld	hl, #1
	ld	-6734(ix), l
	ld	-6733(ix), h
__xcc_L1436:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6736(ix), l
	ld	-6735(ix), h
	.globl __mul16
	ld	l, -6736(ix)
	ld	h, -6735(ix)
	push	hl
	ld	l, -6734(ix)
	ld	h, -6733(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6738(ix), l
	ld	-6737(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6740(ix), l
	ld	-6739(ix), h
	ld	l, -6740(ix)
	ld	h, -6739(ix)
	dec	sp
	dec	sp
	ld	-6742(ix), l
	ld	-6741(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6750(ix), l
	ld	-6749(ix), h
	ld	l, -6742(ix)
	ld	h, -6741(ix)
	push	hl
	ld	l, -6750(ix)
	ld	h, -6749(ix)
	ld	b, l
	pop	hl
__shift_8142:
	ld	a, b
	or	a, a
	jp	z, __sdone_6630
	add	hl, hl
	djnz	__shift_8142
__sdone_6630:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6758(ix), l
	ld	-6757(ix), h
	ld	l, -6758(ix)
	ld	h, -6757(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_64191
	ld	hl, #0
	jp	__cmp_e_45063
__cmp_t_64191:
	ld	hl, #1
__cmp_e_45063:
	dec	sp
	dec	sp
	ld	-6760(ix), l
	ld	-6759(ix), h
	ld	l, -6760(ix)
	ld	h, -6759(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_45520
	ld	hl, #0
	jp	__cmp_e_93320
__cmp_t_45520:
	ld	hl, #1
__cmp_e_93320:
	dec	sp
	dec	sp
	ld	-6762(ix), l
	ld	-6761(ix), h
	ld	l, -6762(ix)
	ld	h, -6761(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1443
	jp	__xcc_L1444
__xcc_L1444:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6764(ix), l
	ld	-6763(ix), h
	ld	l, -6764(ix)
	ld	h, -6763(ix)
	dec	sp
	dec	sp
	ld	-6766(ix), l
	ld	-6765(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6774(ix), l
	ld	-6773(ix), h
	ld	l, -6766(ix)
	ld	h, -6765(ix)
	push	hl
	ld	l, -6774(ix)
	ld	h, -6773(ix)
	ld	b, l
	pop	hl
__shift_6554:
	ld	a, b
	or	a, a
	jp	z, __sdone_538
	add	hl, hl
	djnz	__shift_6554
__sdone_538:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6782(ix), l
	ld	-6781(ix), h
	ld	l, -6782(ix)
	ld	h, -6781(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6784(ix), l
	ld	-6783(ix), h
	ld	l, -6784(ix)
	ld	h, -6783(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87142
	ld	hl, #0
	jp	__cmp_e_31492
__cmp_t_87142:
	ld	hl, #1
__cmp_e_31492:
	dec	sp
	dec	sp
	ld	-6786(ix), l
	ld	-6785(ix), h
	ld	l, -6786(ix)
	ld	h, -6785(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_930
	ld	hl, #0
	jp	__cmp_e_8422
__cmp_t_930:
	ld	hl, #1
__cmp_e_8422:
	dec	sp
	dec	sp
	ld	-6788(ix), l
	ld	-6787(ix), h
	jp	__xcc_L1445
__xcc_L1443:
	ld	hl, #1
	ld	-6788(ix), l
	ld	-6787(ix), h
__xcc_L1445:
	ld	l, -6788(ix)
	ld	h, -6787(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1440
	jp	__xcc_L1441
__xcc_L1440:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6790(ix), l
	ld	-6789(ix), h
	ld	l, -6790(ix)
	ld	h, -6789(ix)
	dec	sp
	dec	sp
	ld	-6792(ix), l
	ld	-6791(ix), h
	jp	__xcc_L1442
__xcc_L1441:
	ld	hl, #1
	ld	-6792(ix), l
	ld	-6791(ix), h
__xcc_L1442:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-6794(ix), l
	ld	-6793(ix), h
	.globl __mul16
	ld	l, -6794(ix)
	ld	h, -6793(ix)
	push	hl
	ld	l, -6792(ix)
	ld	h, -6791(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6796(ix), l
	ld	-6795(ix), h
	ld	l, -6796(ix)
	ld	h, -6795(ix)
	push	hl
	ld	l, -6738(ix)
	ld	h, -6737(ix)
	push	hl
	ld	l, -6712(ix)
	ld	h, -6711(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1446
	dec	sp
	dec	sp
	ld	-6798(ix), l
	ld	-6797(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6800(ix), l
	ld	-6799(ix), h
	ld	l, -6800(ix)
	ld	h, -6799(ix)
	dec	sp
	dec	sp
	ld	-6802(ix), l
	ld	-6801(ix), h
	ld	l, -6802(ix)
	ld	h, -6801(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_34
	ld	hl, #0
	jp	__cmp_e_37685
__cmp_t_34:
	ld	hl, #1
__cmp_e_37685:
	dec	sp
	dec	sp
	ld	-6804(ix), l
	ld	-6803(ix), h
	ld	l, -6804(ix)
	ld	h, -6803(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_27308
	ld	hl, #0
	jp	__cmp_e_58094
__cmp_t_27308:
	ld	hl, #1
__cmp_e_58094:
	dec	sp
	dec	sp
	ld	-6806(ix), l
	ld	-6805(ix), h
	ld	l, -6806(ix)
	ld	h, -6805(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1450
	jp	__xcc_L1451
__xcc_L1451:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6808(ix), l
	ld	-6807(ix), h
	ld	l, -6808(ix)
	ld	h, -6807(ix)
	dec	sp
	dec	sp
	ld	-6810(ix), l
	ld	-6809(ix), h
	ld	l, -6810(ix)
	ld	h, -6809(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6812(ix), l
	ld	-6811(ix), h
	ld	l, -6812(ix)
	ld	h, -6811(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_29780
	ld	hl, #0
	jp	__cmp_e_61708
__cmp_t_29780:
	ld	hl, #1
__cmp_e_61708:
	dec	sp
	dec	sp
	ld	-6814(ix), l
	ld	-6813(ix), h
	ld	l, -6814(ix)
	ld	h, -6813(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_38644
	ld	hl, #0
	jp	__cmp_e_62802
__cmp_t_38644:
	ld	hl, #1
__cmp_e_62802:
	dec	sp
	dec	sp
	ld	-6816(ix), l
	ld	-6815(ix), h
	jp	__xcc_L1452
__xcc_L1450:
	ld	hl, #1
	ld	-6816(ix), l
	ld	-6815(ix), h
__xcc_L1452:
	ld	l, -6816(ix)
	ld	h, -6815(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1447
	jp	__xcc_L1448
__xcc_L1447:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6818(ix), l
	ld	-6817(ix), h
	ld	l, -6818(ix)
	ld	h, -6817(ix)
	dec	sp
	dec	sp
	ld	-6820(ix), l
	ld	-6819(ix), h
	jp	__xcc_L1449
__xcc_L1448:
	ld	hl, #1
	ld	-6820(ix), l
	ld	-6819(ix), h
__xcc_L1449:
	ld	hl, #2
	dec	sp
	dec	sp
	ld	-6822(ix), l
	ld	-6821(ix), h
	.globl __mul16
	ld	l, -6822(ix)
	ld	h, -6821(ix)
	push	hl
	ld	l, -6820(ix)
	ld	h, -6819(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6824(ix), l
	ld	-6823(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6826(ix), l
	ld	-6825(ix), h
	ld	l, -6826(ix)
	ld	h, -6825(ix)
	dec	sp
	dec	sp
	ld	-6828(ix), l
	ld	-6827(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6836(ix), l
	ld	-6835(ix), h
	ld	l, -6828(ix)
	ld	h, -6827(ix)
	push	hl
	ld	l, -6836(ix)
	ld	h, -6835(ix)
	ld	b, l
	pop	hl
__shift_9545:
	ld	a, b
	or	a, a
	jp	z, __sdone_1784
	add	hl, hl
	djnz	__shift_9545
__sdone_1784:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6844(ix), l
	ld	-6843(ix), h
	ld	l, -6844(ix)
	ld	h, -6843(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_96522
	ld	hl, #0
	jp	__cmp_e_41087
__cmp_t_96522:
	ld	hl, #1
__cmp_e_41087:
	dec	sp
	dec	sp
	ld	-6846(ix), l
	ld	-6845(ix), h
	ld	l, -6846(ix)
	ld	h, -6845(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76925
	ld	hl, #0
	jp	__cmp_e_36157
__cmp_t_76925:
	ld	hl, #1
__cmp_e_36157:
	dec	sp
	dec	sp
	ld	-6848(ix), l
	ld	-6847(ix), h
	ld	l, -6848(ix)
	ld	h, -6847(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1456
	jp	__xcc_L1457
__xcc_L1457:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6850(ix), l
	ld	-6849(ix), h
	ld	l, -6850(ix)
	ld	h, -6849(ix)
	dec	sp
	dec	sp
	ld	-6852(ix), l
	ld	-6851(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6860(ix), l
	ld	-6859(ix), h
	ld	l, -6852(ix)
	ld	h, -6851(ix)
	push	hl
	ld	l, -6860(ix)
	ld	h, -6859(ix)
	ld	b, l
	pop	hl
__shift_1735:
	ld	a, b
	or	a, a
	jp	z, __sdone_8602
	add	hl, hl
	djnz	__shift_1735
__sdone_8602:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6868(ix), l
	ld	-6867(ix), h
	ld	l, -6868(ix)
	ld	h, -6867(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6870(ix), l
	ld	-6869(ix), h
	ld	l, -6870(ix)
	ld	h, -6869(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_34492
	ld	hl, #0
	jp	__cmp_e_38548
__cmp_t_34492:
	ld	hl, #1
__cmp_e_38548:
	dec	sp
	dec	sp
	ld	-6872(ix), l
	ld	-6871(ix), h
	ld	l, -6872(ix)
	ld	h, -6871(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30296
	ld	hl, #0
	jp	__cmp_e_98986
__cmp_t_30296:
	ld	hl, #1
__cmp_e_98986:
	dec	sp
	dec	sp
	ld	-6874(ix), l
	ld	-6873(ix), h
	jp	__xcc_L1458
__xcc_L1456:
	ld	hl, #1
	ld	-6874(ix), l
	ld	-6873(ix), h
__xcc_L1458:
	ld	l, -6874(ix)
	ld	h, -6873(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1453
	jp	__xcc_L1454
__xcc_L1453:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6876(ix), l
	ld	-6875(ix), h
	ld	l, -6876(ix)
	ld	h, -6875(ix)
	dec	sp
	dec	sp
	ld	-6878(ix), l
	ld	-6877(ix), h
	jp	__xcc_L1455
__xcc_L1454:
	ld	hl, #1
	ld	-6878(ix), l
	ld	-6877(ix), h
__xcc_L1455:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-6880(ix), l
	ld	-6879(ix), h
	.globl __mul16
	ld	l, -6880(ix)
	ld	h, -6879(ix)
	push	hl
	ld	l, -6878(ix)
	ld	h, -6877(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6882(ix), l
	ld	-6881(ix), h
	ld	l, -6882(ix)
	ld	h, -6881(ix)
	push	hl
	ld	l, -6824(ix)
	ld	h, -6823(ix)
	push	hl
	ld	l, -6798(ix)
	ld	h, -6797(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1431:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1430
	jp	__xcc_L1432
__xcc_L1432:
__xcc_L1341:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1340
	jp	__xcc_L1342
__xcc_L1342:
__xcc_L1219:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1218
	jp	__xcc_L1220
__xcc_L1220:
__xcc_L1459:
__xcc_L1462:
__xcc_L1465:
	ld	hl, #__str_1468
	dec	sp
	dec	sp
	ld	-6884(ix), l
	ld	-6883(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6886(ix), l
	ld	-6885(ix), h
	ld	l, -6886(ix)
	ld	h, -6885(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6890(ix), l
	ld	-6889(ix), h
	ld	l, -6890(ix)
	ld	h, -6889(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51530
	ld	hl, #0
	jp	__cmp_e_10840
__cmp_t_51530:
	ld	hl, #1
__cmp_e_10840:
	dec	sp
	dec	sp
	ld	-6892(ix), l
	ld	-6891(ix), h
	ld	l, -6892(ix)
	ld	h, -6891(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44049
	ld	hl, #0
	jp	__cmp_e_97051
__cmp_t_44049:
	ld	hl, #1
__cmp_e_97051:
	dec	sp
	dec	sp
	ld	-6894(ix), l
	ld	-6893(ix), h
	ld	l, -6894(ix)
	ld	h, -6893(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1472
	jp	__xcc_L1473
__xcc_L1473:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6896(ix), l
	ld	-6895(ix), h
	ld	l, -6896(ix)
	ld	h, -6895(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6900(ix), l
	ld	-6899(ix), h
	ld	l, -6900(ix)
	ld	h, -6899(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6902(ix), l
	ld	-6901(ix), h
	ld	l, -6902(ix)
	ld	h, -6901(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20512
	ld	hl, #0
	jp	__cmp_e_36956
__cmp_t_20512:
	ld	hl, #1
__cmp_e_36956:
	dec	sp
	dec	sp
	ld	-6904(ix), l
	ld	-6903(ix), h
	ld	l, -6904(ix)
	ld	h, -6903(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_77589
	ld	hl, #0
	jp	__cmp_e_7654
__cmp_t_77589:
	ld	hl, #1
__cmp_e_7654:
	dec	sp
	dec	sp
	ld	-6906(ix), l
	ld	-6905(ix), h
	jp	__xcc_L1474
__xcc_L1472:
	ld	hl, #1
	ld	-6906(ix), l
	ld	-6905(ix), h
__xcc_L1474:
	ld	l, -6906(ix)
	ld	h, -6905(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1469
	jp	__xcc_L1470
__xcc_L1469:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6908(ix), l
	ld	-6907(ix), h
	ld	l, -6908(ix)
	ld	h, -6907(ix)
	dec	sp
	dec	sp
	ld	-6910(ix), l
	ld	-6909(ix), h
	jp	__xcc_L1471
__xcc_L1470:
	ld	hl, #1
	ld	-6910(ix), l
	ld	-6909(ix), h
__xcc_L1471:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6912(ix), l
	ld	-6911(ix), h
	.globl __mul16
	ld	l, -6912(ix)
	ld	h, -6911(ix)
	push	hl
	ld	l, -6910(ix)
	ld	h, -6909(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6914(ix), l
	ld	-6913(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6916(ix), l
	ld	-6915(ix), h
	ld	l, -6916(ix)
	ld	h, -6915(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6920(ix), l
	ld	-6919(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6922(ix), l
	ld	-6921(ix), h
	ld	l, -6920(ix)
	ld	h, -6919(ix)
	push	hl
	ld	l, -6922(ix)
	ld	h, -6921(ix)
	ld	b, l
	pop	hl
__shift_8448:
	ld	a, b
	or	a, a
	jp	z, __sdone_4872
	add	hl, hl
	djnz	__shift_8448
__sdone_4872:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6926(ix), l
	ld	-6925(ix), h
	ld	l, -6926(ix)
	ld	h, -6925(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_32428
	ld	hl, #0
	jp	__cmp_e_68482
__cmp_t_32428:
	ld	hl, #1
__cmp_e_68482:
	dec	sp
	dec	sp
	ld	-6928(ix), l
	ld	-6927(ix), h
	ld	l, -6928(ix)
	ld	h, -6927(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_32557
	ld	hl, #0
	jp	__cmp_e_76088
__cmp_t_32557:
	ld	hl, #1
__cmp_e_76088:
	dec	sp
	dec	sp
	ld	-6930(ix), l
	ld	-6929(ix), h
	ld	l, -6930(ix)
	ld	h, -6929(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1478
	jp	__xcc_L1479
__xcc_L1479:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6932(ix), l
	ld	-6931(ix), h
	ld	l, -6932(ix)
	ld	h, -6931(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6936(ix), l
	ld	-6935(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6938(ix), l
	ld	-6937(ix), h
	ld	l, -6936(ix)
	ld	h, -6935(ix)
	push	hl
	ld	l, -6938(ix)
	ld	h, -6937(ix)
	ld	b, l
	pop	hl
__shift_6576:
	ld	a, b
	or	a, a
	jp	z, __sdone_2337
	add	hl, hl
	djnz	__shift_6576
__sdone_2337:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6942(ix), l
	ld	-6941(ix), h
	ld	l, -6942(ix)
	ld	h, -6941(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6944(ix), l
	ld	-6943(ix), h
	ld	l, -6944(ix)
	ld	h, -6943(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37797
	ld	hl, #0
	jp	__cmp_e_65220
__cmp_t_37797:
	ld	hl, #1
__cmp_e_65220:
	dec	sp
	dec	sp
	ld	-6946(ix), l
	ld	-6945(ix), h
	ld	l, -6946(ix)
	ld	h, -6945(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25139
	ld	hl, #0
	jp	__cmp_e_33694
__cmp_t_25139:
	ld	hl, #1
__cmp_e_33694:
	dec	sp
	dec	sp
	ld	-6948(ix), l
	ld	-6947(ix), h
	jp	__xcc_L1480
__xcc_L1478:
	ld	hl, #1
	ld	-6948(ix), l
	ld	-6947(ix), h
__xcc_L1480:
	ld	l, -6948(ix)
	ld	h, -6947(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1475
	jp	__xcc_L1476
__xcc_L1475:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6950(ix), l
	ld	-6949(ix), h
	ld	l, -6950(ix)
	ld	h, -6949(ix)
	dec	sp
	dec	sp
	ld	-6952(ix), l
	ld	-6951(ix), h
	jp	__xcc_L1477
__xcc_L1476:
	ld	hl, #1
	ld	-6952(ix), l
	ld	-6951(ix), h
__xcc_L1477:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6954(ix), l
	ld	-6953(ix), h
	.globl __mul16
	ld	l, -6954(ix)
	ld	h, -6953(ix)
	push	hl
	ld	l, -6952(ix)
	ld	h, -6951(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6956(ix), l
	ld	-6955(ix), h
	ld	l, -6956(ix)
	ld	h, -6955(ix)
	push	hl
	ld	l, -6914(ix)
	ld	h, -6913(ix)
	push	hl
	ld	l, -6884(ix)
	ld	h, -6883(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1481
	dec	sp
	dec	sp
	ld	-6958(ix), l
	ld	-6957(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6960(ix), l
	ld	-6959(ix), h
	ld	l, -6960(ix)
	ld	h, -6959(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6964(ix), l
	ld	-6963(ix), h
	ld	l, -6964(ix)
	ld	h, -6963(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87005
	ld	hl, #0
	jp	__cmp_e_38014
__cmp_t_87005:
	ld	hl, #1
__cmp_e_38014:
	dec	sp
	dec	sp
	ld	-6966(ix), l
	ld	-6965(ix), h
	ld	l, -6966(ix)
	ld	h, -6965(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74782
	ld	hl, #0
	jp	__cmp_e_80282
__cmp_t_74782:
	ld	hl, #1
__cmp_e_80282:
	dec	sp
	dec	sp
	ld	-6968(ix), l
	ld	-6967(ix), h
	ld	l, -6968(ix)
	ld	h, -6967(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1485
	jp	__xcc_L1486
__xcc_L1486:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6970(ix), l
	ld	-6969(ix), h
	ld	l, -6970(ix)
	ld	h, -6969(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6974(ix), l
	ld	-6973(ix), h
	ld	l, -6974(ix)
	ld	h, -6973(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6976(ix), l
	ld	-6975(ix), h
	ld	l, -6976(ix)
	ld	h, -6975(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90523
	ld	hl, #0
	jp	__cmp_e_42869
__cmp_t_90523:
	ld	hl, #1
__cmp_e_42869:
	dec	sp
	dec	sp
	ld	-6978(ix), l
	ld	-6977(ix), h
	ld	l, -6978(ix)
	ld	h, -6977(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_35236
	ld	hl, #0
	jp	__cmp_e_25015
__cmp_t_35236:
	ld	hl, #1
__cmp_e_25015:
	dec	sp
	dec	sp
	ld	-6980(ix), l
	ld	-6979(ix), h
	jp	__xcc_L1487
__xcc_L1485:
	ld	hl, #1
	ld	-6980(ix), l
	ld	-6979(ix), h
__xcc_L1487:
	ld	l, -6980(ix)
	ld	h, -6979(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1482
	jp	__xcc_L1483
__xcc_L1482:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6982(ix), l
	ld	-6981(ix), h
	ld	l, -6982(ix)
	ld	h, -6981(ix)
	dec	sp
	dec	sp
	ld	-6984(ix), l
	ld	-6983(ix), h
	jp	__xcc_L1484
__xcc_L1483:
	ld	hl, #1
	ld	-6984(ix), l
	ld	-6983(ix), h
__xcc_L1484:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-6986(ix), l
	ld	-6985(ix), h
	.globl __mul16
	ld	l, -6986(ix)
	ld	h, -6985(ix)
	push	hl
	ld	l, -6984(ix)
	ld	h, -6983(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6988(ix), l
	ld	-6987(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-6990(ix), l
	ld	-6989(ix), h
	ld	l, -6990(ix)
	ld	h, -6989(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-6994(ix), l
	ld	-6993(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-6996(ix), l
	ld	-6995(ix), h
	ld	l, -6994(ix)
	ld	h, -6993(ix)
	push	hl
	ld	l, -6996(ix)
	ld	h, -6995(ix)
	ld	b, l
	pop	hl
__shift_1417:
	ld	a, b
	or	a, a
	jp	z, __sdone_1884
	add	hl, hl
	djnz	__shift_1417
__sdone_1884:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7000(ix), l
	ld	-6999(ix), h
	ld	l, -7000(ix)
	ld	h, -6999(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_40353
	ld	hl, #0
	jp	__cmp_e_49299
__cmp_t_40353:
	ld	hl, #1
__cmp_e_49299:
	dec	sp
	dec	sp
	ld	-7002(ix), l
	ld	-7001(ix), h
	ld	l, -7002(ix)
	ld	h, -7001(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92724
	ld	hl, #0
	jp	__cmp_e_754
__cmp_t_92724:
	ld	hl, #1
__cmp_e_754:
	dec	sp
	dec	sp
	ld	-7004(ix), l
	ld	-7003(ix), h
	ld	l, -7004(ix)
	ld	h, -7003(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1491
	jp	__xcc_L1492
__xcc_L1492:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7006(ix), l
	ld	-7005(ix), h
	ld	l, -7006(ix)
	ld	h, -7005(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7010(ix), l
	ld	-7009(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7012(ix), l
	ld	-7011(ix), h
	ld	l, -7010(ix)
	ld	h, -7009(ix)
	push	hl
	ld	l, -7012(ix)
	ld	h, -7011(ix)
	ld	b, l
	pop	hl
__shift_6350:
	ld	a, b
	or	a, a
	jp	z, __sdone_3236
	add	hl, hl
	djnz	__shift_6350
__sdone_3236:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7016(ix), l
	ld	-7015(ix), h
	ld	l, -7016(ix)
	ld	h, -7015(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7018(ix), l
	ld	-7017(ix), h
	ld	l, -7018(ix)
	ld	h, -7017(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37710
	ld	hl, #0
	jp	__cmp_e_40292
__cmp_t_37710:
	ld	hl, #1
__cmp_e_40292:
	dec	sp
	dec	sp
	ld	-7020(ix), l
	ld	-7019(ix), h
	ld	l, -7020(ix)
	ld	h, -7019(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_37242
	ld	hl, #0
	jp	__cmp_e_6158
__cmp_t_37242:
	ld	hl, #1
__cmp_e_6158:
	dec	sp
	dec	sp
	ld	-7022(ix), l
	ld	-7021(ix), h
	jp	__xcc_L1493
__xcc_L1491:
	ld	hl, #1
	ld	-7022(ix), l
	ld	-7021(ix), h
__xcc_L1493:
	ld	l, -7022(ix)
	ld	h, -7021(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1488
	jp	__xcc_L1489
__xcc_L1488:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7024(ix), l
	ld	-7023(ix), h
	ld	l, -7024(ix)
	ld	h, -7023(ix)
	dec	sp
	dec	sp
	ld	-7026(ix), l
	ld	-7025(ix), h
	jp	__xcc_L1490
__xcc_L1489:
	ld	hl, #1
	ld	-7026(ix), l
	ld	-7025(ix), h
__xcc_L1490:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7028(ix), l
	ld	-7027(ix), h
	.globl __mul16
	ld	l, -7028(ix)
	ld	h, -7027(ix)
	push	hl
	ld	l, -7026(ix)
	ld	h, -7025(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7030(ix), l
	ld	-7029(ix), h
	ld	l, -7030(ix)
	ld	h, -7029(ix)
	push	hl
	ld	l, -6988(ix)
	ld	h, -6987(ix)
	push	hl
	ld	l, -6958(ix)
	ld	h, -6957(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1466:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1465
	jp	__xcc_L1467
__xcc_L1467:
__xcc_L1494:
	ld	hl, #__str_1497
	dec	sp
	dec	sp
	ld	-7032(ix), l
	ld	-7031(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7034(ix), l
	ld	-7033(ix), h
	ld	l, -7034(ix)
	ld	h, -7033(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7038(ix), l
	ld	-7037(ix), h
	ld	l, -7038(ix)
	ld	h, -7037(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35164
	ld	hl, #0
	jp	__cmp_e_86023
__cmp_t_35164:
	ld	hl, #1
__cmp_e_86023:
	dec	sp
	dec	sp
	ld	-7040(ix), l
	ld	-7039(ix), h
	ld	l, -7040(ix)
	ld	h, -7039(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74641
	ld	hl, #0
	jp	__cmp_e_67721
__cmp_t_74641:
	ld	hl, #1
__cmp_e_67721:
	dec	sp
	dec	sp
	ld	-7042(ix), l
	ld	-7041(ix), h
	ld	l, -7042(ix)
	ld	h, -7041(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1501
	jp	__xcc_L1502
__xcc_L1502:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7044(ix), l
	ld	-7043(ix), h
	ld	l, -7044(ix)
	ld	h, -7043(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7048(ix), l
	ld	-7047(ix), h
	ld	l, -7048(ix)
	ld	h, -7047(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7050(ix), l
	ld	-7049(ix), h
	ld	l, -7050(ix)
	ld	h, -7049(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_62111
	ld	hl, #0
	jp	__cmp_e_17569
__cmp_t_62111:
	ld	hl, #1
__cmp_e_17569:
	dec	sp
	dec	sp
	ld	-7052(ix), l
	ld	-7051(ix), h
	ld	l, -7052(ix)
	ld	h, -7051(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_46410
	ld	hl, #0
	jp	__cmp_e_16260
__cmp_t_46410:
	ld	hl, #1
__cmp_e_16260:
	dec	sp
	dec	sp
	ld	-7054(ix), l
	ld	-7053(ix), h
	jp	__xcc_L1503
__xcc_L1501:
	ld	hl, #1
	ld	-7054(ix), l
	ld	-7053(ix), h
__xcc_L1503:
	ld	l, -7054(ix)
	ld	h, -7053(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1498
	jp	__xcc_L1499
__xcc_L1498:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7056(ix), l
	ld	-7055(ix), h
	ld	l, -7056(ix)
	ld	h, -7055(ix)
	dec	sp
	dec	sp
	ld	-7058(ix), l
	ld	-7057(ix), h
	jp	__xcc_L1500
__xcc_L1499:
	ld	hl, #1
	ld	-7058(ix), l
	ld	-7057(ix), h
__xcc_L1500:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7060(ix), l
	ld	-7059(ix), h
	.globl __mul16
	ld	l, -7060(ix)
	ld	h, -7059(ix)
	push	hl
	ld	l, -7058(ix)
	ld	h, -7057(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7062(ix), l
	ld	-7061(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7064(ix), l
	ld	-7063(ix), h
	ld	l, -7064(ix)
	ld	h, -7063(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7068(ix), l
	ld	-7067(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7070(ix), l
	ld	-7069(ix), h
	ld	l, -7068(ix)
	ld	h, -7067(ix)
	push	hl
	ld	l, -7070(ix)
	ld	h, -7069(ix)
	ld	b, l
	pop	hl
__shift_2790:
	ld	a, b
	or	a, a
	jp	z, __sdone_7902
	add	hl, hl
	djnz	__shift_2790
__sdone_7902:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7074(ix), l
	ld	-7073(ix), h
	ld	l, -7074(ix)
	ld	h, -7073(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49955
	ld	hl, #0
	jp	__cmp_e_86147
__cmp_t_49955:
	ld	hl, #1
__cmp_e_86147:
	dec	sp
	dec	sp
	ld	-7076(ix), l
	ld	-7075(ix), h
	ld	l, -7076(ix)
	ld	h, -7075(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25916
	ld	hl, #0
	jp	__cmp_e_41089
__cmp_t_25916:
	ld	hl, #1
__cmp_e_41089:
	dec	sp
	dec	sp
	ld	-7078(ix), l
	ld	-7077(ix), h
	ld	l, -7078(ix)
	ld	h, -7077(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1507
	jp	__xcc_L1508
__xcc_L1508:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7080(ix), l
	ld	-7079(ix), h
	ld	l, -7080(ix)
	ld	h, -7079(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7084(ix), l
	ld	-7083(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7086(ix), l
	ld	-7085(ix), h
	ld	l, -7084(ix)
	ld	h, -7083(ix)
	push	hl
	ld	l, -7086(ix)
	ld	h, -7085(ix)
	ld	b, l
	pop	hl
__shift_2781:
	ld	a, b
	or	a, a
	jp	z, __sdone_6439
	add	hl, hl
	djnz	__shift_2781
__sdone_6439:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7090(ix), l
	ld	-7089(ix), h
	ld	l, -7090(ix)
	ld	h, -7089(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7092(ix), l
	ld	-7091(ix), h
	ld	l, -7092(ix)
	ld	h, -7091(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_83958
	ld	hl, #0
	jp	__cmp_e_18017
__cmp_t_83958:
	ld	hl, #1
__cmp_e_18017:
	dec	sp
	dec	sp
	ld	-7094(ix), l
	ld	-7093(ix), h
	ld	l, -7094(ix)
	ld	h, -7093(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_57806
	ld	hl, #0
	jp	__cmp_e_65375
__cmp_t_57806:
	ld	hl, #1
__cmp_e_65375:
	dec	sp
	dec	sp
	ld	-7096(ix), l
	ld	-7095(ix), h
	jp	__xcc_L1509
__xcc_L1507:
	ld	hl, #1
	ld	-7096(ix), l
	ld	-7095(ix), h
__xcc_L1509:
	ld	l, -7096(ix)
	ld	h, -7095(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1504
	jp	__xcc_L1505
__xcc_L1504:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7098(ix), l
	ld	-7097(ix), h
	ld	l, -7098(ix)
	ld	h, -7097(ix)
	dec	sp
	dec	sp
	ld	-7100(ix), l
	ld	-7099(ix), h
	jp	__xcc_L1506
__xcc_L1505:
	ld	hl, #1
	ld	-7100(ix), l
	ld	-7099(ix), h
__xcc_L1506:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7102(ix), l
	ld	-7101(ix), h
	.globl __mul16
	ld	l, -7102(ix)
	ld	h, -7101(ix)
	push	hl
	ld	l, -7100(ix)
	ld	h, -7099(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7104(ix), l
	ld	-7103(ix), h
	ld	l, -7104(ix)
	ld	h, -7103(ix)
	push	hl
	ld	l, -7062(ix)
	ld	h, -7061(ix)
	push	hl
	ld	l, -7032(ix)
	ld	h, -7031(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1510
	dec	sp
	dec	sp
	ld	-7106(ix), l
	ld	-7105(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7108(ix), l
	ld	-7107(ix), h
	ld	l, -7108(ix)
	ld	h, -7107(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7112(ix), l
	ld	-7111(ix), h
	ld	l, -7112(ix)
	ld	h, -7111(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99901
	ld	hl, #0
	jp	__cmp_e_14511
__cmp_t_99901:
	ld	hl, #1
__cmp_e_14511:
	dec	sp
	dec	sp
	ld	-7114(ix), l
	ld	-7113(ix), h
	ld	l, -7114(ix)
	ld	h, -7113(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14674
	ld	hl, #0
	jp	__cmp_e_8978
__cmp_t_14674:
	ld	hl, #1
__cmp_e_8978:
	dec	sp
	dec	sp
	ld	-7116(ix), l
	ld	-7115(ix), h
	ld	l, -7116(ix)
	ld	h, -7115(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1514
	jp	__xcc_L1515
__xcc_L1515:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7118(ix), l
	ld	-7117(ix), h
	ld	l, -7118(ix)
	ld	h, -7117(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7122(ix), l
	ld	-7121(ix), h
	ld	l, -7122(ix)
	ld	h, -7121(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7124(ix), l
	ld	-7123(ix), h
	ld	l, -7124(ix)
	ld	h, -7123(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_15265
	ld	hl, #0
	jp	__cmp_e_77377
__cmp_t_15265:
	ld	hl, #1
__cmp_e_77377:
	dec	sp
	dec	sp
	ld	-7126(ix), l
	ld	-7125(ix), h
	ld	l, -7126(ix)
	ld	h, -7125(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_38566
	ld	hl, #0
	jp	__cmp_e_52976
__cmp_t_38566:
	ld	hl, #1
__cmp_e_52976:
	dec	sp
	dec	sp
	ld	-7128(ix), l
	ld	-7127(ix), h
	jp	__xcc_L1516
__xcc_L1514:
	ld	hl, #1
	ld	-7128(ix), l
	ld	-7127(ix), h
__xcc_L1516:
	ld	l, -7128(ix)
	ld	h, -7127(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1511
	jp	__xcc_L1512
__xcc_L1511:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7130(ix), l
	ld	-7129(ix), h
	ld	l, -7130(ix)
	ld	h, -7129(ix)
	dec	sp
	dec	sp
	ld	-7132(ix), l
	ld	-7131(ix), h
	jp	__xcc_L1513
__xcc_L1512:
	ld	hl, #1
	ld	-7132(ix), l
	ld	-7131(ix), h
__xcc_L1513:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7134(ix), l
	ld	-7133(ix), h
	.globl __mul16
	ld	l, -7134(ix)
	ld	h, -7133(ix)
	push	hl
	ld	l, -7132(ix)
	ld	h, -7131(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7136(ix), l
	ld	-7135(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7138(ix), l
	ld	-7137(ix), h
	ld	l, -7138(ix)
	ld	h, -7137(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7142(ix), l
	ld	-7141(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7144(ix), l
	ld	-7143(ix), h
	ld	l, -7142(ix)
	ld	h, -7141(ix)
	push	hl
	ld	l, -7144(ix)
	ld	h, -7143(ix)
	ld	b, l
	pop	hl
__shift_7669:
	ld	a, b
	or	a, a
	jp	z, __sdone_2161
	add	hl, hl
	djnz	__shift_7669
__sdone_2161:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7148(ix), l
	ld	-7147(ix), h
	ld	l, -7148(ix)
	ld	h, -7147(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59134
	ld	hl, #0
	jp	__cmp_e_52833
__cmp_t_59134:
	ld	hl, #1
__cmp_e_52833:
	dec	sp
	dec	sp
	ld	-7150(ix), l
	ld	-7149(ix), h
	ld	l, -7150(ix)
	ld	h, -7149(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_94536
	ld	hl, #0
	jp	__cmp_e_50127
__cmp_t_94536:
	ld	hl, #1
__cmp_e_50127:
	dec	sp
	dec	sp
	ld	-7152(ix), l
	ld	-7151(ix), h
	ld	l, -7152(ix)
	ld	h, -7151(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1520
	jp	__xcc_L1521
__xcc_L1521:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7154(ix), l
	ld	-7153(ix), h
	ld	l, -7154(ix)
	ld	h, -7153(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7158(ix), l
	ld	-7157(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7160(ix), l
	ld	-7159(ix), h
	ld	l, -7158(ix)
	ld	h, -7157(ix)
	push	hl
	ld	l, -7160(ix)
	ld	h, -7159(ix)
	ld	b, l
	pop	hl
__shift_6906:
	ld	a, b
	or	a, a
	jp	z, __sdone_2999
	add	hl, hl
	djnz	__shift_6906
__sdone_2999:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7164(ix), l
	ld	-7163(ix), h
	ld	l, -7164(ix)
	ld	h, -7163(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7166(ix), l
	ld	-7165(ix), h
	ld	l, -7166(ix)
	ld	h, -7165(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_67697
	ld	hl, #0
	jp	__cmp_e_83316
__cmp_t_67697:
	ld	hl, #1
__cmp_e_83316:
	dec	sp
	dec	sp
	ld	-7168(ix), l
	ld	-7167(ix), h
	ld	l, -7168(ix)
	ld	h, -7167(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89260
	ld	hl, #0
	jp	__cmp_e_66839
__cmp_t_89260:
	ld	hl, #1
__cmp_e_66839:
	dec	sp
	dec	sp
	ld	-7170(ix), l
	ld	-7169(ix), h
	jp	__xcc_L1522
__xcc_L1520:
	ld	hl, #1
	ld	-7170(ix), l
	ld	-7169(ix), h
__xcc_L1522:
	ld	l, -7170(ix)
	ld	h, -7169(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1517
	jp	__xcc_L1518
__xcc_L1517:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7172(ix), l
	ld	-7171(ix), h
	ld	l, -7172(ix)
	ld	h, -7171(ix)
	dec	sp
	dec	sp
	ld	-7174(ix), l
	ld	-7173(ix), h
	jp	__xcc_L1519
__xcc_L1518:
	ld	hl, #1
	ld	-7174(ix), l
	ld	-7173(ix), h
__xcc_L1519:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7176(ix), l
	ld	-7175(ix), h
	.globl __mul16
	ld	l, -7176(ix)
	ld	h, -7175(ix)
	push	hl
	ld	l, -7174(ix)
	ld	h, -7173(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7178(ix), l
	ld	-7177(ix), h
	ld	l, -7178(ix)
	ld	h, -7177(ix)
	push	hl
	ld	l, -7136(ix)
	ld	h, -7135(ix)
	push	hl
	ld	l, -7106(ix)
	ld	h, -7105(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1495:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1494
	jp	__xcc_L1496
__xcc_L1496:
__xcc_L1523:
	ld	hl, #__str_1526
	dec	sp
	dec	sp
	ld	-7180(ix), l
	ld	-7179(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7182(ix), l
	ld	-7181(ix), h
	ld	l, -7182(ix)
	ld	h, -7181(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7186(ix), l
	ld	-7185(ix), h
	ld	l, -7186(ix)
	ld	h, -7185(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87570
	ld	hl, #0
	jp	__cmp_e_55567
__cmp_t_87570:
	ld	hl, #1
__cmp_e_55567:
	dec	sp
	dec	sp
	ld	-7188(ix), l
	ld	-7187(ix), h
	ld	l, -7188(ix)
	ld	h, -7187(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52986
	ld	hl, #0
	jp	__cmp_e_13486
__cmp_t_52986:
	ld	hl, #1
__cmp_e_13486:
	dec	sp
	dec	sp
	ld	-7190(ix), l
	ld	-7189(ix), h
	ld	l, -7190(ix)
	ld	h, -7189(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1530
	jp	__xcc_L1531
__xcc_L1531:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7192(ix), l
	ld	-7191(ix), h
	ld	l, -7192(ix)
	ld	h, -7191(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7196(ix), l
	ld	-7195(ix), h
	ld	l, -7196(ix)
	ld	h, -7195(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7198(ix), l
	ld	-7197(ix), h
	ld	l, -7198(ix)
	ld	h, -7197(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_13008
	ld	hl, #0
	jp	__cmp_e_35767
__cmp_t_13008:
	ld	hl, #1
__cmp_e_35767:
	dec	sp
	dec	sp
	ld	-7200(ix), l
	ld	-7199(ix), h
	ld	l, -7200(ix)
	ld	h, -7199(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_46277
	ld	hl, #0
	jp	__cmp_e_96966
__cmp_t_46277:
	ld	hl, #1
__cmp_e_96966:
	dec	sp
	dec	sp
	ld	-7202(ix), l
	ld	-7201(ix), h
	jp	__xcc_L1532
__xcc_L1530:
	ld	hl, #1
	ld	-7202(ix), l
	ld	-7201(ix), h
__xcc_L1532:
	ld	l, -7202(ix)
	ld	h, -7201(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1527
	jp	__xcc_L1528
__xcc_L1527:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7204(ix), l
	ld	-7203(ix), h
	ld	l, -7204(ix)
	ld	h, -7203(ix)
	dec	sp
	dec	sp
	ld	-7206(ix), l
	ld	-7205(ix), h
	jp	__xcc_L1529
__xcc_L1528:
	ld	hl, #1
	ld	-7206(ix), l
	ld	-7205(ix), h
__xcc_L1529:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7208(ix), l
	ld	-7207(ix), h
	.globl __mul16
	ld	l, -7208(ix)
	ld	h, -7207(ix)
	push	hl
	ld	l, -7206(ix)
	ld	h, -7205(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7210(ix), l
	ld	-7209(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7212(ix), l
	ld	-7211(ix), h
	ld	l, -7212(ix)
	ld	h, -7211(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7216(ix), l
	ld	-7215(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7220(ix), l
	ld	-7219(ix), h
	ld	l, -7216(ix)
	ld	h, -7215(ix)
	push	hl
	ld	l, -7220(ix)
	ld	h, -7219(ix)
	ld	b, l
	pop	hl
__shift_136:
	ld	a, b
	or	a, a
	jp	z, __sdone_435
	add	hl, hl
	djnz	__shift_136
__sdone_435:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7224(ix), l
	ld	-7223(ix), h
	ld	l, -7224(ix)
	ld	h, -7223(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78693
	ld	hl, #0
	jp	__cmp_e_70037
__cmp_t_78693:
	ld	hl, #1
__cmp_e_70037:
	dec	sp
	dec	sp
	ld	-7226(ix), l
	ld	-7225(ix), h
	ld	l, -7226(ix)
	ld	h, -7225(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34946
	ld	hl, #0
	jp	__cmp_e_9719
__cmp_t_34946:
	ld	hl, #1
__cmp_e_9719:
	dec	sp
	dec	sp
	ld	-7228(ix), l
	ld	-7227(ix), h
	ld	l, -7228(ix)
	ld	h, -7227(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1536
	jp	__xcc_L1537
__xcc_L1537:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7230(ix), l
	ld	-7229(ix), h
	ld	l, -7230(ix)
	ld	h, -7229(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7234(ix), l
	ld	-7233(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7238(ix), l
	ld	-7237(ix), h
	ld	l, -7234(ix)
	ld	h, -7233(ix)
	push	hl
	ld	l, -7238(ix)
	ld	h, -7237(ix)
	ld	b, l
	pop	hl
__shift_5367:
	ld	a, b
	or	a, a
	jp	z, __sdone_212
	add	hl, hl
	djnz	__shift_5367
__sdone_212:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7242(ix), l
	ld	-7241(ix), h
	ld	l, -7242(ix)
	ld	h, -7241(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7244(ix), l
	ld	-7243(ix), h
	ld	l, -7244(ix)
	ld	h, -7243(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_87096
	ld	hl, #0
	jp	__cmp_e_33934
__cmp_t_87096:
	ld	hl, #1
__cmp_e_33934:
	dec	sp
	dec	sp
	ld	-7246(ix), l
	ld	-7245(ix), h
	ld	l, -7246(ix)
	ld	h, -7245(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19540
	ld	hl, #0
	jp	__cmp_e_21117
__cmp_t_19540:
	ld	hl, #1
__cmp_e_21117:
	dec	sp
	dec	sp
	ld	-7248(ix), l
	ld	-7247(ix), h
	jp	__xcc_L1538
__xcc_L1536:
	ld	hl, #1
	ld	-7248(ix), l
	ld	-7247(ix), h
__xcc_L1538:
	ld	l, -7248(ix)
	ld	h, -7247(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1533
	jp	__xcc_L1534
__xcc_L1533:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7250(ix), l
	ld	-7249(ix), h
	ld	l, -7250(ix)
	ld	h, -7249(ix)
	dec	sp
	dec	sp
	ld	-7252(ix), l
	ld	-7251(ix), h
	jp	__xcc_L1535
__xcc_L1534:
	ld	hl, #1
	ld	-7252(ix), l
	ld	-7251(ix), h
__xcc_L1535:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7254(ix), l
	ld	-7253(ix), h
	.globl __mul16
	ld	l, -7254(ix)
	ld	h, -7253(ix)
	push	hl
	ld	l, -7252(ix)
	ld	h, -7251(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7256(ix), l
	ld	-7255(ix), h
	ld	l, -7256(ix)
	ld	h, -7255(ix)
	push	hl
	ld	l, -7210(ix)
	ld	h, -7209(ix)
	push	hl
	ld	l, -7180(ix)
	ld	h, -7179(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1539
	dec	sp
	dec	sp
	ld	-7258(ix), l
	ld	-7257(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7260(ix), l
	ld	-7259(ix), h
	ld	l, -7260(ix)
	ld	h, -7259(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7264(ix), l
	ld	-7263(ix), h
	ld	l, -7264(ix)
	ld	h, -7263(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_26095
	ld	hl, #0
	jp	__cmp_e_78674
__cmp_t_26095:
	ld	hl, #1
__cmp_e_78674:
	dec	sp
	dec	sp
	ld	-7266(ix), l
	ld	-7265(ix), h
	ld	l, -7266(ix)
	ld	h, -7265(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73950
	ld	hl, #0
	jp	__cmp_e_20631
__cmp_t_73950:
	ld	hl, #1
__cmp_e_20631:
	dec	sp
	dec	sp
	ld	-7268(ix), l
	ld	-7267(ix), h
	ld	l, -7268(ix)
	ld	h, -7267(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1543
	jp	__xcc_L1544
__xcc_L1544:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7270(ix), l
	ld	-7269(ix), h
	ld	l, -7270(ix)
	ld	h, -7269(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7274(ix), l
	ld	-7273(ix), h
	ld	l, -7274(ix)
	ld	h, -7273(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7276(ix), l
	ld	-7275(ix), h
	ld	l, -7276(ix)
	ld	h, -7275(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_28802
	ld	hl, #0
	jp	__cmp_e_27208
__cmp_t_28802:
	ld	hl, #1
__cmp_e_27208:
	dec	sp
	dec	sp
	ld	-7278(ix), l
	ld	-7277(ix), h
	ld	l, -7278(ix)
	ld	h, -7277(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_93630
	ld	hl, #0
	jp	__cmp_e_12851
__cmp_t_93630:
	ld	hl, #1
__cmp_e_12851:
	dec	sp
	dec	sp
	ld	-7280(ix), l
	ld	-7279(ix), h
	jp	__xcc_L1545
__xcc_L1543:
	ld	hl, #1
	ld	-7280(ix), l
	ld	-7279(ix), h
__xcc_L1545:
	ld	l, -7280(ix)
	ld	h, -7279(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1540
	jp	__xcc_L1541
__xcc_L1540:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7282(ix), l
	ld	-7281(ix), h
	ld	l, -7282(ix)
	ld	h, -7281(ix)
	dec	sp
	dec	sp
	ld	-7284(ix), l
	ld	-7283(ix), h
	jp	__xcc_L1542
__xcc_L1541:
	ld	hl, #1
	ld	-7284(ix), l
	ld	-7283(ix), h
__xcc_L1542:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7286(ix), l
	ld	-7285(ix), h
	.globl __mul16
	ld	l, -7286(ix)
	ld	h, -7285(ix)
	push	hl
	ld	l, -7284(ix)
	ld	h, -7283(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7288(ix), l
	ld	-7287(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7290(ix), l
	ld	-7289(ix), h
	ld	l, -7290(ix)
	ld	h, -7289(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7294(ix), l
	ld	-7293(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7298(ix), l
	ld	-7297(ix), h
	ld	l, -7294(ix)
	ld	h, -7293(ix)
	push	hl
	ld	l, -7298(ix)
	ld	h, -7297(ix)
	ld	b, l
	pop	hl
__shift_525:
	ld	a, b
	or	a, a
	jp	z, __sdone_9242
	add	hl, hl
	djnz	__shift_525
__sdone_9242:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7302(ix), l
	ld	-7301(ix), h
	ld	l, -7302(ix)
	ld	h, -7301(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_79690
	ld	hl, #0
	jp	__cmp_e_14447
__cmp_t_79690:
	ld	hl, #1
__cmp_e_14447:
	dec	sp
	dec	sp
	ld	-7304(ix), l
	ld	-7303(ix), h
	ld	l, -7304(ix)
	ld	h, -7303(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71161
	ld	hl, #0
	jp	__cmp_e_32676
__cmp_t_71161:
	ld	hl, #1
__cmp_e_32676:
	dec	sp
	dec	sp
	ld	-7306(ix), l
	ld	-7305(ix), h
	ld	l, -7306(ix)
	ld	h, -7305(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1549
	jp	__xcc_L1550
__xcc_L1550:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7308(ix), l
	ld	-7307(ix), h
	ld	l, -7308(ix)
	ld	h, -7307(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7312(ix), l
	ld	-7311(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7316(ix), l
	ld	-7315(ix), h
	ld	l, -7312(ix)
	ld	h, -7311(ix)
	push	hl
	ld	l, -7316(ix)
	ld	h, -7315(ix)
	ld	b, l
	pop	hl
__shift_7934:
	ld	a, b
	or	a, a
	jp	z, __sdone_4169
	add	hl, hl
	djnz	__shift_7934
__sdone_4169:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7320(ix), l
	ld	-7319(ix), h
	ld	l, -7320(ix)
	ld	h, -7319(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7322(ix), l
	ld	-7321(ix), h
	ld	l, -7322(ix)
	ld	h, -7321(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84795
	ld	hl, #0
	jp	__cmp_e_90563
__cmp_t_84795:
	ld	hl, #1
__cmp_e_90563:
	dec	sp
	dec	sp
	ld	-7324(ix), l
	ld	-7323(ix), h
	ld	l, -7324(ix)
	ld	h, -7323(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81135
	ld	hl, #0
	jp	__cmp_e_54931
__cmp_t_81135:
	ld	hl, #1
__cmp_e_54931:
	dec	sp
	dec	sp
	ld	-7326(ix), l
	ld	-7325(ix), h
	jp	__xcc_L1551
__xcc_L1549:
	ld	hl, #1
	ld	-7326(ix), l
	ld	-7325(ix), h
__xcc_L1551:
	ld	l, -7326(ix)
	ld	h, -7325(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1546
	jp	__xcc_L1547
__xcc_L1546:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7328(ix), l
	ld	-7327(ix), h
	ld	l, -7328(ix)
	ld	h, -7327(ix)
	dec	sp
	dec	sp
	ld	-7330(ix), l
	ld	-7329(ix), h
	jp	__xcc_L1548
__xcc_L1547:
	ld	hl, #1
	ld	-7330(ix), l
	ld	-7329(ix), h
__xcc_L1548:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7332(ix), l
	ld	-7331(ix), h
	.globl __mul16
	ld	l, -7332(ix)
	ld	h, -7331(ix)
	push	hl
	ld	l, -7330(ix)
	ld	h, -7329(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7334(ix), l
	ld	-7333(ix), h
	ld	l, -7334(ix)
	ld	h, -7333(ix)
	push	hl
	ld	l, -7288(ix)
	ld	h, -7287(ix)
	push	hl
	ld	l, -7258(ix)
	ld	h, -7257(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1524:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1523
	jp	__xcc_L1525
__xcc_L1525:
__xcc_L1552:
	ld	hl, #__str_1555
	dec	sp
	dec	sp
	ld	-7336(ix), l
	ld	-7335(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7338(ix), l
	ld	-7337(ix), h
	ld	l, -7338(ix)
	ld	h, -7337(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7342(ix), l
	ld	-7341(ix), h
	ld	l, -7342(ix)
	ld	h, -7341(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_27351
	ld	hl, #0
	jp	__cmp_e_76180
__cmp_t_27351:
	ld	hl, #1
__cmp_e_76180:
	dec	sp
	dec	sp
	ld	-7344(ix), l
	ld	-7343(ix), h
	ld	l, -7344(ix)
	ld	h, -7343(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_41320
	ld	hl, #0
	jp	__cmp_e_62297
__cmp_t_41320:
	ld	hl, #1
__cmp_e_62297:
	dec	sp
	dec	sp
	ld	-7346(ix), l
	ld	-7345(ix), h
	ld	l, -7346(ix)
	ld	h, -7345(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1559
	jp	__xcc_L1560
__xcc_L1560:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7348(ix), l
	ld	-7347(ix), h
	ld	l, -7348(ix)
	ld	h, -7347(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7352(ix), l
	ld	-7351(ix), h
	ld	l, -7352(ix)
	ld	h, -7351(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7354(ix), l
	ld	-7353(ix), h
	ld	l, -7354(ix)
	ld	h, -7353(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85900
	ld	hl, #0
	jp	__cmp_e_36688
__cmp_t_85900:
	ld	hl, #1
__cmp_e_36688:
	dec	sp
	dec	sp
	ld	-7356(ix), l
	ld	-7355(ix), h
	ld	l, -7356(ix)
	ld	h, -7355(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_28861
	ld	hl, #0
	jp	__cmp_e_72996
__cmp_t_28861:
	ld	hl, #1
__cmp_e_72996:
	dec	sp
	dec	sp
	ld	-7358(ix), l
	ld	-7357(ix), h
	jp	__xcc_L1561
__xcc_L1559:
	ld	hl, #1
	ld	-7358(ix), l
	ld	-7357(ix), h
__xcc_L1561:
	ld	l, -7358(ix)
	ld	h, -7357(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1556
	jp	__xcc_L1557
__xcc_L1556:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7360(ix), l
	ld	-7359(ix), h
	ld	l, -7360(ix)
	ld	h, -7359(ix)
	dec	sp
	dec	sp
	ld	-7362(ix), l
	ld	-7361(ix), h
	jp	__xcc_L1558
__xcc_L1557:
	ld	hl, #1
	ld	-7362(ix), l
	ld	-7361(ix), h
__xcc_L1558:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7364(ix), l
	ld	-7363(ix), h
	.globl __mul16
	ld	l, -7364(ix)
	ld	h, -7363(ix)
	push	hl
	ld	l, -7362(ix)
	ld	h, -7361(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7366(ix), l
	ld	-7365(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7368(ix), l
	ld	-7367(ix), h
	ld	l, -7368(ix)
	ld	h, -7367(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7372(ix), l
	ld	-7371(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7380(ix), l
	ld	-7379(ix), h
	ld	l, -7372(ix)
	ld	h, -7371(ix)
	push	hl
	ld	l, -7380(ix)
	ld	h, -7379(ix)
	ld	b, l
	pop	hl
__shift_6974:
	ld	a, b
	or	a, a
	jp	z, __sdone_8401
	add	hl, hl
	djnz	__shift_6974
__sdone_8401:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7388(ix), l
	ld	-7387(ix), h
	ld	l, -7388(ix)
	ld	h, -7387(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_94114
	ld	hl, #0
	jp	__cmp_e_13069
__cmp_t_94114:
	ld	hl, #1
__cmp_e_13069:
	dec	sp
	dec	sp
	ld	-7390(ix), l
	ld	-7389(ix), h
	ld	l, -7390(ix)
	ld	h, -7389(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43428
	ld	hl, #0
	jp	__cmp_e_84416
__cmp_t_43428:
	ld	hl, #1
__cmp_e_84416:
	dec	sp
	dec	sp
	ld	-7392(ix), l
	ld	-7391(ix), h
	ld	l, -7392(ix)
	ld	h, -7391(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1565
	jp	__xcc_L1566
__xcc_L1566:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7394(ix), l
	ld	-7393(ix), h
	ld	l, -7394(ix)
	ld	h, -7393(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7398(ix), l
	ld	-7397(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7406(ix), l
	ld	-7405(ix), h
	ld	l, -7398(ix)
	ld	h, -7397(ix)
	push	hl
	ld	l, -7406(ix)
	ld	h, -7405(ix)
	ld	b, l
	pop	hl
__shift_52:
	ld	a, b
	or	a, a
	jp	z, __sdone_8582
	add	hl, hl
	djnz	__shift_52
__sdone_8582:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7414(ix), l
	ld	-7413(ix), h
	ld	l, -7414(ix)
	ld	h, -7413(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7416(ix), l
	ld	-7415(ix), h
	ld	l, -7416(ix)
	ld	h, -7415(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_11625
	ld	hl, #0
	jp	__cmp_e_43682
__cmp_t_11625:
	ld	hl, #1
__cmp_e_43682:
	dec	sp
	dec	sp
	ld	-7418(ix), l
	ld	-7417(ix), h
	ld	l, -7418(ix)
	ld	h, -7417(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_1433
	ld	hl, #0
	jp	__cmp_e_38502
__cmp_t_1433:
	ld	hl, #1
__cmp_e_38502:
	dec	sp
	dec	sp
	ld	-7420(ix), l
	ld	-7419(ix), h
	jp	__xcc_L1567
__xcc_L1565:
	ld	hl, #1
	ld	-7420(ix), l
	ld	-7419(ix), h
__xcc_L1567:
	ld	l, -7420(ix)
	ld	h, -7419(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1562
	jp	__xcc_L1563
__xcc_L1562:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7422(ix), l
	ld	-7421(ix), h
	ld	l, -7422(ix)
	ld	h, -7421(ix)
	dec	sp
	dec	sp
	ld	-7424(ix), l
	ld	-7423(ix), h
	jp	__xcc_L1564
__xcc_L1563:
	ld	hl, #1
	ld	-7424(ix), l
	ld	-7423(ix), h
__xcc_L1564:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-7426(ix), l
	ld	-7425(ix), h
	.globl __mul16
	ld	l, -7426(ix)
	ld	h, -7425(ix)
	push	hl
	ld	l, -7424(ix)
	ld	h, -7423(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7428(ix), l
	ld	-7427(ix), h
	ld	l, -7428(ix)
	ld	h, -7427(ix)
	push	hl
	ld	l, -7366(ix)
	ld	h, -7365(ix)
	push	hl
	ld	l, -7336(ix)
	ld	h, -7335(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1568
	dec	sp
	dec	sp
	ld	-7430(ix), l
	ld	-7429(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7432(ix), l
	ld	-7431(ix), h
	ld	l, -7432(ix)
	ld	h, -7431(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7436(ix), l
	ld	-7435(ix), h
	ld	l, -7436(ix)
	ld	h, -7435(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59277
	ld	hl, #0
	jp	__cmp_e_81123
__cmp_t_59277:
	ld	hl, #1
__cmp_e_81123:
	dec	sp
	dec	sp
	ld	-7438(ix), l
	ld	-7437(ix), h
	ld	l, -7438(ix)
	ld	h, -7437(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_52949
	ld	hl, #0
	jp	__cmp_e_46790
__cmp_t_52949:
	ld	hl, #1
__cmp_e_46790:
	dec	sp
	dec	sp
	ld	-7440(ix), l
	ld	-7439(ix), h
	ld	l, -7440(ix)
	ld	h, -7439(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1572
	jp	__xcc_L1573
__xcc_L1573:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7442(ix), l
	ld	-7441(ix), h
	ld	l, -7442(ix)
	ld	h, -7441(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7446(ix), l
	ld	-7445(ix), h
	ld	l, -7446(ix)
	ld	h, -7445(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7448(ix), l
	ld	-7447(ix), h
	ld	l, -7448(ix)
	ld	h, -7447(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30151
	ld	hl, #0
	jp	__cmp_e_97235
__cmp_t_30151:
	ld	hl, #1
__cmp_e_97235:
	dec	sp
	dec	sp
	ld	-7450(ix), l
	ld	-7449(ix), h
	ld	l, -7450(ix)
	ld	h, -7449(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_30960
	ld	hl, #0
	jp	__cmp_e_14946
__cmp_t_30960:
	ld	hl, #1
__cmp_e_14946:
	dec	sp
	dec	sp
	ld	-7452(ix), l
	ld	-7451(ix), h
	jp	__xcc_L1574
__xcc_L1572:
	ld	hl, #1
	ld	-7452(ix), l
	ld	-7451(ix), h
__xcc_L1574:
	ld	l, -7452(ix)
	ld	h, -7451(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1569
	jp	__xcc_L1570
__xcc_L1569:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7454(ix), l
	ld	-7453(ix), h
	ld	l, -7454(ix)
	ld	h, -7453(ix)
	dec	sp
	dec	sp
	ld	-7456(ix), l
	ld	-7455(ix), h
	jp	__xcc_L1571
__xcc_L1570:
	ld	hl, #1
	ld	-7456(ix), l
	ld	-7455(ix), h
__xcc_L1571:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7458(ix), l
	ld	-7457(ix), h
	.globl __mul16
	ld	l, -7458(ix)
	ld	h, -7457(ix)
	push	hl
	ld	l, -7456(ix)
	ld	h, -7455(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7460(ix), l
	ld	-7459(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7462(ix), l
	ld	-7461(ix), h
	ld	l, -7462(ix)
	ld	h, -7461(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7466(ix), l
	ld	-7465(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7474(ix), l
	ld	-7473(ix), h
	ld	l, -7466(ix)
	ld	h, -7465(ix)
	push	hl
	ld	l, -7474(ix)
	ld	h, -7473(ix)
	ld	b, l
	pop	hl
__shift_7799:
	ld	a, b
	or	a, a
	jp	z, __sdone_8447
	add	hl, hl
	djnz	__shift_7799
__sdone_8447:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7482(ix), l
	ld	-7481(ix), h
	ld	l, -7482(ix)
	ld	h, -7481(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_86229
	ld	hl, #0
	jp	__cmp_e_31502
__cmp_t_86229:
	ld	hl, #1
__cmp_e_31502:
	dec	sp
	dec	sp
	ld	-7484(ix), l
	ld	-7483(ix), h
	ld	l, -7484(ix)
	ld	h, -7483(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4628
	ld	hl, #0
	jp	__cmp_e_27549
__cmp_t_4628:
	ld	hl, #1
__cmp_e_27549:
	dec	sp
	dec	sp
	ld	-7486(ix), l
	ld	-7485(ix), h
	ld	l, -7486(ix)
	ld	h, -7485(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1578
	jp	__xcc_L1579
__xcc_L1579:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7488(ix), l
	ld	-7487(ix), h
	ld	l, -7488(ix)
	ld	h, -7487(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7492(ix), l
	ld	-7491(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7500(ix), l
	ld	-7499(ix), h
	ld	l, -7492(ix)
	ld	h, -7491(ix)
	push	hl
	ld	l, -7500(ix)
	ld	h, -7499(ix)
	ld	b, l
	pop	hl
__shift_3799:
	ld	a, b
	or	a, a
	jp	z, __sdone_528
	add	hl, hl
	djnz	__shift_3799
__sdone_528:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7508(ix), l
	ld	-7507(ix), h
	ld	l, -7508(ix)
	ld	h, -7507(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7510(ix), l
	ld	-7509(ix), h
	ld	l, -7510(ix)
	ld	h, -7509(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80589
	ld	hl, #0
	jp	__cmp_e_39013
__cmp_t_80589:
	ld	hl, #1
__cmp_e_39013:
	dec	sp
	dec	sp
	ld	-7512(ix), l
	ld	-7511(ix), h
	ld	l, -7512(ix)
	ld	h, -7511(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79876
	ld	hl, #0
	jp	__cmp_e_67563
__cmp_t_79876:
	ld	hl, #1
__cmp_e_67563:
	dec	sp
	dec	sp
	ld	-7514(ix), l
	ld	-7513(ix), h
	jp	__xcc_L1580
__xcc_L1578:
	ld	hl, #1
	ld	-7514(ix), l
	ld	-7513(ix), h
__xcc_L1580:
	ld	l, -7514(ix)
	ld	h, -7513(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1575
	jp	__xcc_L1576
__xcc_L1575:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7516(ix), l
	ld	-7515(ix), h
	ld	l, -7516(ix)
	ld	h, -7515(ix)
	dec	sp
	dec	sp
	ld	-7518(ix), l
	ld	-7517(ix), h
	jp	__xcc_L1577
__xcc_L1576:
	ld	hl, #1
	ld	-7518(ix), l
	ld	-7517(ix), h
__xcc_L1577:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-7520(ix), l
	ld	-7519(ix), h
	.globl __mul16
	ld	l, -7520(ix)
	ld	h, -7519(ix)
	push	hl
	ld	l, -7518(ix)
	ld	h, -7517(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7522(ix), l
	ld	-7521(ix), h
	ld	l, -7522(ix)
	ld	h, -7521(ix)
	push	hl
	ld	l, -7460(ix)
	ld	h, -7459(ix)
	push	hl
	ld	l, -7430(ix)
	ld	h, -7429(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1553:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1552
	jp	__xcc_L1554
__xcc_L1554:
__xcc_L1463:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1462
	jp	__xcc_L1464
__xcc_L1464:
__xcc_L1581:
__xcc_L1584:
	ld	hl, #__str_1587
	dec	sp
	dec	sp
	ld	-7524(ix), l
	ld	-7523(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7526(ix), l
	ld	-7525(ix), h
	ld	l, -7526(ix)
	ld	h, -7525(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7530(ix), l
	ld	-7529(ix), h
	ld	l, -7530(ix)
	ld	h, -7529(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3766
	ld	hl, #0
	jp	__cmp_e_73990
__cmp_t_3766:
	ld	hl, #1
__cmp_e_73990:
	dec	sp
	dec	sp
	ld	-7532(ix), l
	ld	-7531(ix), h
	ld	l, -7532(ix)
	ld	h, -7531(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96984
	ld	hl, #0
	jp	__cmp_e_47194
__cmp_t_96984:
	ld	hl, #1
__cmp_e_47194:
	dec	sp
	dec	sp
	ld	-7534(ix), l
	ld	-7533(ix), h
	ld	l, -7534(ix)
	ld	h, -7533(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1591
	jp	__xcc_L1592
__xcc_L1592:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7536(ix), l
	ld	-7535(ix), h
	ld	l, -7536(ix)
	ld	h, -7535(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7540(ix), l
	ld	-7539(ix), h
	ld	l, -7540(ix)
	ld	h, -7539(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7542(ix), l
	ld	-7541(ix), h
	ld	l, -7542(ix)
	ld	h, -7541(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74759
	ld	hl, #0
	jp	__cmp_e_47036
__cmp_t_74759:
	ld	hl, #1
__cmp_e_47036:
	dec	sp
	dec	sp
	ld	-7544(ix), l
	ld	-7543(ix), h
	ld	l, -7544(ix)
	ld	h, -7543(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_35776
	ld	hl, #0
	jp	__cmp_e_86384
__cmp_t_35776:
	ld	hl, #1
__cmp_e_86384:
	dec	sp
	dec	sp
	ld	-7546(ix), l
	ld	-7545(ix), h
	jp	__xcc_L1593
__xcc_L1591:
	ld	hl, #1
	ld	-7546(ix), l
	ld	-7545(ix), h
__xcc_L1593:
	ld	l, -7546(ix)
	ld	h, -7545(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1588
	jp	__xcc_L1589
__xcc_L1588:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7548(ix), l
	ld	-7547(ix), h
	ld	l, -7548(ix)
	ld	h, -7547(ix)
	dec	sp
	dec	sp
	ld	-7550(ix), l
	ld	-7549(ix), h
	jp	__xcc_L1590
__xcc_L1589:
	ld	hl, #1
	ld	-7550(ix), l
	ld	-7549(ix), h
__xcc_L1590:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7552(ix), l
	ld	-7551(ix), h
	.globl __mul16
	ld	l, -7552(ix)
	ld	h, -7551(ix)
	push	hl
	ld	l, -7550(ix)
	ld	h, -7549(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7554(ix), l
	ld	-7553(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7556(ix), l
	ld	-7555(ix), h
	ld	l, -7556(ix)
	ld	h, -7555(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7560(ix), l
	ld	-7559(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7562(ix), l
	ld	-7561(ix), h
	ld	l, -7560(ix)
	ld	h, -7559(ix)
	push	hl
	ld	l, -7562(ix)
	ld	h, -7561(ix)
	ld	b, l
	pop	hl
__shift_7071:
	ld	a, b
	or	a, a
	jp	z, __sdone_7209
	add	hl, hl
	djnz	__shift_7071
__sdone_7209:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7566(ix), l
	ld	-7565(ix), h
	ld	l, -7566(ix)
	ld	h, -7565(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41238
	ld	hl, #0
	jp	__cmp_e_82700
__cmp_t_41238:
	ld	hl, #1
__cmp_e_82700:
	dec	sp
	dec	sp
	ld	-7568(ix), l
	ld	-7567(ix), h
	ld	l, -7568(ix)
	ld	h, -7567(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34684
	ld	hl, #0
	jp	__cmp_e_10539
__cmp_t_34684:
	ld	hl, #1
__cmp_e_10539:
	dec	sp
	dec	sp
	ld	-7570(ix), l
	ld	-7569(ix), h
	ld	l, -7570(ix)
	ld	h, -7569(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1597
	jp	__xcc_L1598
__xcc_L1598:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7572(ix), l
	ld	-7571(ix), h
	ld	l, -7572(ix)
	ld	h, -7571(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7576(ix), l
	ld	-7575(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7578(ix), l
	ld	-7577(ix), h
	ld	l, -7576(ix)
	ld	h, -7575(ix)
	push	hl
	ld	l, -7578(ix)
	ld	h, -7577(ix)
	ld	b, l
	pop	hl
__shift_9490:
	ld	a, b
	or	a, a
	jp	z, __sdone_4835
	add	hl, hl
	djnz	__shift_9490
__sdone_4835:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7582(ix), l
	ld	-7581(ix), h
	ld	l, -7582(ix)
	ld	h, -7581(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7584(ix), l
	ld	-7583(ix), h
	ld	l, -7584(ix)
	ld	h, -7583(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_7775
	ld	hl, #0
	jp	__cmp_e_60450
__cmp_t_7775:
	ld	hl, #1
__cmp_e_60450:
	dec	sp
	dec	sp
	ld	-7586(ix), l
	ld	-7585(ix), h
	ld	l, -7586(ix)
	ld	h, -7585(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_79781
	ld	hl, #0
	jp	__cmp_e_11926
__cmp_t_79781:
	ld	hl, #1
__cmp_e_11926:
	dec	sp
	dec	sp
	ld	-7588(ix), l
	ld	-7587(ix), h
	jp	__xcc_L1599
__xcc_L1597:
	ld	hl, #1
	ld	-7588(ix), l
	ld	-7587(ix), h
__xcc_L1599:
	ld	l, -7588(ix)
	ld	h, -7587(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1594
	jp	__xcc_L1595
__xcc_L1594:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7590(ix), l
	ld	-7589(ix), h
	ld	l, -7590(ix)
	ld	h, -7589(ix)
	dec	sp
	dec	sp
	ld	-7592(ix), l
	ld	-7591(ix), h
	jp	__xcc_L1596
__xcc_L1595:
	ld	hl, #1
	ld	-7592(ix), l
	ld	-7591(ix), h
__xcc_L1596:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7594(ix), l
	ld	-7593(ix), h
	.globl __mul16
	ld	l, -7594(ix)
	ld	h, -7593(ix)
	push	hl
	ld	l, -7592(ix)
	ld	h, -7591(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7596(ix), l
	ld	-7595(ix), h
	ld	l, -7596(ix)
	ld	h, -7595(ix)
	push	hl
	ld	l, -7554(ix)
	ld	h, -7553(ix)
	push	hl
	ld	l, -7524(ix)
	ld	h, -7523(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1600
	dec	sp
	dec	sp
	ld	-7598(ix), l
	ld	-7597(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7600(ix), l
	ld	-7599(ix), h
	ld	l, -7600(ix)
	ld	h, -7599(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7604(ix), l
	ld	-7603(ix), h
	ld	l, -7604(ix)
	ld	h, -7603(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5250
	ld	hl, #0
	jp	__cmp_e_82362
__cmp_t_5250:
	ld	hl, #1
__cmp_e_82362:
	dec	sp
	dec	sp
	ld	-7606(ix), l
	ld	-7605(ix), h
	ld	l, -7606(ix)
	ld	h, -7605(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43428
	ld	hl, #0
	jp	__cmp_e_9878
__cmp_t_43428:
	ld	hl, #1
__cmp_e_9878:
	dec	sp
	dec	sp
	ld	-7608(ix), l
	ld	-7607(ix), h
	ld	l, -7608(ix)
	ld	h, -7607(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1604
	jp	__xcc_L1605
__xcc_L1605:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7610(ix), l
	ld	-7609(ix), h
	ld	l, -7610(ix)
	ld	h, -7609(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7614(ix), l
	ld	-7613(ix), h
	ld	l, -7614(ix)
	ld	h, -7613(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7616(ix), l
	ld	-7615(ix), h
	ld	l, -7616(ix)
	ld	h, -7615(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_26264
	ld	hl, #0
	jp	__cmp_e_53579
__cmp_t_26264:
	ld	hl, #1
__cmp_e_53579:
	dec	sp
	dec	sp
	ld	-7618(ix), l
	ld	-7617(ix), h
	ld	l, -7618(ix)
	ld	h, -7617(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16758
	ld	hl, #0
	jp	__cmp_e_6853
__cmp_t_16758:
	ld	hl, #1
__cmp_e_6853:
	dec	sp
	dec	sp
	ld	-7620(ix), l
	ld	-7619(ix), h
	jp	__xcc_L1606
__xcc_L1604:
	ld	hl, #1
	ld	-7620(ix), l
	ld	-7619(ix), h
__xcc_L1606:
	ld	l, -7620(ix)
	ld	h, -7619(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1601
	jp	__xcc_L1602
__xcc_L1601:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7622(ix), l
	ld	-7621(ix), h
	ld	l, -7622(ix)
	ld	h, -7621(ix)
	dec	sp
	dec	sp
	ld	-7624(ix), l
	ld	-7623(ix), h
	jp	__xcc_L1603
__xcc_L1602:
	ld	hl, #1
	ld	-7624(ix), l
	ld	-7623(ix), h
__xcc_L1603:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7626(ix), l
	ld	-7625(ix), h
	.globl __mul16
	ld	l, -7626(ix)
	ld	h, -7625(ix)
	push	hl
	ld	l, -7624(ix)
	ld	h, -7623(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7628(ix), l
	ld	-7627(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7630(ix), l
	ld	-7629(ix), h
	ld	l, -7630(ix)
	ld	h, -7629(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7634(ix), l
	ld	-7633(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7636(ix), l
	ld	-7635(ix), h
	ld	l, -7634(ix)
	ld	h, -7633(ix)
	push	hl
	ld	l, -7636(ix)
	ld	h, -7635(ix)
	ld	b, l
	pop	hl
__shift_8944:
	ld	a, b
	or	a, a
	jp	z, __sdone_6634
	add	hl, hl
	djnz	__shift_8944
__sdone_6634:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7640(ix), l
	ld	-7639(ix), h
	ld	l, -7640(ix)
	ld	h, -7639(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90769
	ld	hl, #0
	jp	__cmp_e_12711
__cmp_t_90769:
	ld	hl, #1
__cmp_e_12711:
	dec	sp
	dec	sp
	ld	-7642(ix), l
	ld	-7641(ix), h
	ld	l, -7642(ix)
	ld	h, -7641(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_86977
	ld	hl, #0
	jp	__cmp_e_4105
__cmp_t_86977:
	ld	hl, #1
__cmp_e_4105:
	dec	sp
	dec	sp
	ld	-7644(ix), l
	ld	-7643(ix), h
	ld	l, -7644(ix)
	ld	h, -7643(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1610
	jp	__xcc_L1611
__xcc_L1611:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7646(ix), l
	ld	-7645(ix), h
	ld	l, -7646(ix)
	ld	h, -7645(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7650(ix), l
	ld	-7649(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7652(ix), l
	ld	-7651(ix), h
	ld	l, -7650(ix)
	ld	h, -7649(ix)
	push	hl
	ld	l, -7652(ix)
	ld	h, -7651(ix)
	ld	b, l
	pop	hl
__shift_6257:
	ld	a, b
	or	a, a
	jp	z, __sdone_1736
	add	hl, hl
	djnz	__shift_6257
__sdone_1736:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7656(ix), l
	ld	-7655(ix), h
	ld	l, -7656(ix)
	ld	h, -7655(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7658(ix), l
	ld	-7657(ix), h
	ld	l, -7658(ix)
	ld	h, -7657(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51142
	ld	hl, #0
	jp	__cmp_e_12034
__cmp_t_51142:
	ld	hl, #1
__cmp_e_12034:
	dec	sp
	dec	sp
	ld	-7660(ix), l
	ld	-7659(ix), h
	ld	l, -7660(ix)
	ld	h, -7659(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_64472
	ld	hl, #0
	jp	__cmp_e_74565
__cmp_t_64472:
	ld	hl, #1
__cmp_e_74565:
	dec	sp
	dec	sp
	ld	-7662(ix), l
	ld	-7661(ix), h
	jp	__xcc_L1612
__xcc_L1610:
	ld	hl, #1
	ld	-7662(ix), l
	ld	-7661(ix), h
__xcc_L1612:
	ld	l, -7662(ix)
	ld	h, -7661(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1607
	jp	__xcc_L1608
__xcc_L1607:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7664(ix), l
	ld	-7663(ix), h
	ld	l, -7664(ix)
	ld	h, -7663(ix)
	dec	sp
	dec	sp
	ld	-7666(ix), l
	ld	-7665(ix), h
	jp	__xcc_L1609
__xcc_L1608:
	ld	hl, #1
	ld	-7666(ix), l
	ld	-7665(ix), h
__xcc_L1609:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7668(ix), l
	ld	-7667(ix), h
	.globl __mul16
	ld	l, -7668(ix)
	ld	h, -7667(ix)
	push	hl
	ld	l, -7666(ix)
	ld	h, -7665(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7670(ix), l
	ld	-7669(ix), h
	ld	l, -7670(ix)
	ld	h, -7669(ix)
	push	hl
	ld	l, -7628(ix)
	ld	h, -7627(ix)
	push	hl
	ld	l, -7598(ix)
	ld	h, -7597(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1585:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1584
	jp	__xcc_L1586
__xcc_L1586:
__xcc_L1613:
	ld	hl, #__str_1616
	dec	sp
	dec	sp
	ld	-7672(ix), l
	ld	-7671(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7674(ix), l
	ld	-7673(ix), h
	ld	l, -7674(ix)
	ld	h, -7673(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7678(ix), l
	ld	-7677(ix), h
	ld	l, -7678(ix)
	ld	h, -7677(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65595
	ld	hl, #0
	jp	__cmp_e_5710
__cmp_t_65595:
	ld	hl, #1
__cmp_e_5710:
	dec	sp
	dec	sp
	ld	-7680(ix), l
	ld	-7679(ix), h
	ld	l, -7680(ix)
	ld	h, -7679(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_57265
	ld	hl, #0
	jp	__cmp_e_16632
__cmp_t_57265:
	ld	hl, #1
__cmp_e_16632:
	dec	sp
	dec	sp
	ld	-7682(ix), l
	ld	-7681(ix), h
	ld	l, -7682(ix)
	ld	h, -7681(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1620
	jp	__xcc_L1621
__xcc_L1621:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7684(ix), l
	ld	-7683(ix), h
	ld	l, -7684(ix)
	ld	h, -7683(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7688(ix), l
	ld	-7687(ix), h
	ld	l, -7688(ix)
	ld	h, -7687(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7690(ix), l
	ld	-7689(ix), h
	ld	l, -7690(ix)
	ld	h, -7689(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16249
	ld	hl, #0
	jp	__cmp_e_3107
__cmp_t_16249:
	ld	hl, #1
__cmp_e_3107:
	dec	sp
	dec	sp
	ld	-7692(ix), l
	ld	-7691(ix), h
	ld	l, -7692(ix)
	ld	h, -7691(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_81467
	ld	hl, #0
	jp	__cmp_e_40376
__cmp_t_81467:
	ld	hl, #1
__cmp_e_40376:
	dec	sp
	dec	sp
	ld	-7694(ix), l
	ld	-7693(ix), h
	jp	__xcc_L1622
__xcc_L1620:
	ld	hl, #1
	ld	-7694(ix), l
	ld	-7693(ix), h
__xcc_L1622:
	ld	l, -7694(ix)
	ld	h, -7693(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1617
	jp	__xcc_L1618
__xcc_L1617:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7696(ix), l
	ld	-7695(ix), h
	ld	l, -7696(ix)
	ld	h, -7695(ix)
	dec	sp
	dec	sp
	ld	-7698(ix), l
	ld	-7697(ix), h
	jp	__xcc_L1619
__xcc_L1618:
	ld	hl, #1
	ld	-7698(ix), l
	ld	-7697(ix), h
__xcc_L1619:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7700(ix), l
	ld	-7699(ix), h
	.globl __mul16
	ld	l, -7700(ix)
	ld	h, -7699(ix)
	push	hl
	ld	l, -7698(ix)
	ld	h, -7697(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7702(ix), l
	ld	-7701(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7704(ix), l
	ld	-7703(ix), h
	ld	l, -7704(ix)
	ld	h, -7703(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7708(ix), l
	ld	-7707(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7710(ix), l
	ld	-7709(ix), h
	ld	l, -7708(ix)
	ld	h, -7707(ix)
	push	hl
	ld	l, -7710(ix)
	ld	h, -7709(ix)
	ld	b, l
	pop	hl
__shift_3558:
	ld	a, b
	or	a, a
	jp	z, __sdone_7601
	add	hl, hl
	djnz	__shift_3558
__sdone_7601:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7714(ix), l
	ld	-7713(ix), h
	ld	l, -7714(ix)
	ld	h, -7713(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_52302
	ld	hl, #0
	jp	__cmp_e_85160
__cmp_t_52302:
	ld	hl, #1
__cmp_e_85160:
	dec	sp
	dec	sp
	ld	-7716(ix), l
	ld	-7715(ix), h
	ld	l, -7716(ix)
	ld	h, -7715(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_59963
	ld	hl, #0
	jp	__cmp_e_12082
__cmp_t_59963:
	ld	hl, #1
__cmp_e_12082:
	dec	sp
	dec	sp
	ld	-7718(ix), l
	ld	-7717(ix), h
	ld	l, -7718(ix)
	ld	h, -7717(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1626
	jp	__xcc_L1627
__xcc_L1627:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7720(ix), l
	ld	-7719(ix), h
	ld	l, -7720(ix)
	ld	h, -7719(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7724(ix), l
	ld	-7723(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7726(ix), l
	ld	-7725(ix), h
	ld	l, -7724(ix)
	ld	h, -7723(ix)
	push	hl
	ld	l, -7726(ix)
	ld	h, -7725(ix)
	ld	b, l
	pop	hl
__shift_5038:
	ld	a, b
	or	a, a
	jp	z, __sdone_6227
	add	hl, hl
	djnz	__shift_5038
__sdone_6227:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7730(ix), l
	ld	-7729(ix), h
	ld	l, -7730(ix)
	ld	h, -7729(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7732(ix), l
	ld	-7731(ix), h
	ld	l, -7732(ix)
	ld	h, -7731(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_82014
	ld	hl, #0
	jp	__cmp_e_11796
__cmp_t_82014:
	ld	hl, #1
__cmp_e_11796:
	dec	sp
	dec	sp
	ld	-7734(ix), l
	ld	-7733(ix), h
	ld	l, -7734(ix)
	ld	h, -7733(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_9433
	ld	hl, #0
	jp	__cmp_e_90958
__cmp_t_9433:
	ld	hl, #1
__cmp_e_90958:
	dec	sp
	dec	sp
	ld	-7736(ix), l
	ld	-7735(ix), h
	jp	__xcc_L1628
__xcc_L1626:
	ld	hl, #1
	ld	-7736(ix), l
	ld	-7735(ix), h
__xcc_L1628:
	ld	l, -7736(ix)
	ld	h, -7735(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1623
	jp	__xcc_L1624
__xcc_L1623:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7738(ix), l
	ld	-7737(ix), h
	ld	l, -7738(ix)
	ld	h, -7737(ix)
	dec	sp
	dec	sp
	ld	-7740(ix), l
	ld	-7739(ix), h
	jp	__xcc_L1625
__xcc_L1624:
	ld	hl, #1
	ld	-7740(ix), l
	ld	-7739(ix), h
__xcc_L1625:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7742(ix), l
	ld	-7741(ix), h
	.globl __mul16
	ld	l, -7742(ix)
	ld	h, -7741(ix)
	push	hl
	ld	l, -7740(ix)
	ld	h, -7739(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7744(ix), l
	ld	-7743(ix), h
	ld	l, -7744(ix)
	ld	h, -7743(ix)
	push	hl
	ld	l, -7702(ix)
	ld	h, -7701(ix)
	push	hl
	ld	l, -7672(ix)
	ld	h, -7671(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1629
	dec	sp
	dec	sp
	ld	-7746(ix), l
	ld	-7745(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7748(ix), l
	ld	-7747(ix), h
	ld	l, -7748(ix)
	ld	h, -7747(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7752(ix), l
	ld	-7751(ix), h
	ld	l, -7752(ix)
	ld	h, -7751(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_8430
	ld	hl, #0
	jp	__cmp_e_16554
__cmp_t_8430:
	ld	hl, #1
__cmp_e_16554:
	dec	sp
	dec	sp
	ld	-7754(ix), l
	ld	-7753(ix), h
	ld	l, -7754(ix)
	ld	h, -7753(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3669
	ld	hl, #0
	jp	__cmp_e_95407
__cmp_t_3669:
	ld	hl, #1
__cmp_e_95407:
	dec	sp
	dec	sp
	ld	-7756(ix), l
	ld	-7755(ix), h
	ld	l, -7756(ix)
	ld	h, -7755(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1633
	jp	__xcc_L1634
__xcc_L1634:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7758(ix), l
	ld	-7757(ix), h
	ld	l, -7758(ix)
	ld	h, -7757(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7762(ix), l
	ld	-7761(ix), h
	ld	l, -7762(ix)
	ld	h, -7761(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7764(ix), l
	ld	-7763(ix), h
	ld	l, -7764(ix)
	ld	h, -7763(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_20659
	ld	hl, #0
	jp	__cmp_e_79927
__cmp_t_20659:
	ld	hl, #1
__cmp_e_79927:
	dec	sp
	dec	sp
	ld	-7766(ix), l
	ld	-7765(ix), h
	ld	l, -7766(ix)
	ld	h, -7765(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73495
	ld	hl, #0
	jp	__cmp_e_71801
__cmp_t_73495:
	ld	hl, #1
__cmp_e_71801:
	dec	sp
	dec	sp
	ld	-7768(ix), l
	ld	-7767(ix), h
	jp	__xcc_L1635
__xcc_L1633:
	ld	hl, #1
	ld	-7768(ix), l
	ld	-7767(ix), h
__xcc_L1635:
	ld	l, -7768(ix)
	ld	h, -7767(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1630
	jp	__xcc_L1631
__xcc_L1630:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7770(ix), l
	ld	-7769(ix), h
	ld	l, -7770(ix)
	ld	h, -7769(ix)
	dec	sp
	dec	sp
	ld	-7772(ix), l
	ld	-7771(ix), h
	jp	__xcc_L1632
__xcc_L1631:
	ld	hl, #1
	ld	-7772(ix), l
	ld	-7771(ix), h
__xcc_L1632:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7774(ix), l
	ld	-7773(ix), h
	.globl __mul16
	ld	l, -7774(ix)
	ld	h, -7773(ix)
	push	hl
	ld	l, -7772(ix)
	ld	h, -7771(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7776(ix), l
	ld	-7775(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7778(ix), l
	ld	-7777(ix), h
	ld	l, -7778(ix)
	ld	h, -7777(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7782(ix), l
	ld	-7781(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7784(ix), l
	ld	-7783(ix), h
	ld	l, -7782(ix)
	ld	h, -7781(ix)
	push	hl
	ld	l, -7784(ix)
	ld	h, -7783(ix)
	ld	b, l
	pop	hl
__shift_8313:
	ld	a, b
	or	a, a
	jp	z, __sdone_7967
	add	hl, hl
	djnz	__shift_8313
__sdone_7967:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7788(ix), l
	ld	-7787(ix), h
	ld	l, -7788(ix)
	ld	h, -7787(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_62718
	ld	hl, #0
	jp	__cmp_e_90260
__cmp_t_62718:
	ld	hl, #1
__cmp_e_90260:
	dec	sp
	dec	sp
	ld	-7790(ix), l
	ld	-7789(ix), h
	ld	l, -7790(ix)
	ld	h, -7789(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_60029
	ld	hl, #0
	jp	__cmp_e_36335
__cmp_t_60029:
	ld	hl, #1
__cmp_e_36335:
	dec	sp
	dec	sp
	ld	-7792(ix), l
	ld	-7791(ix), h
	ld	l, -7792(ix)
	ld	h, -7791(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1639
	jp	__xcc_L1640
__xcc_L1640:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7794(ix), l
	ld	-7793(ix), h
	ld	l, -7794(ix)
	ld	h, -7793(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7798(ix), l
	ld	-7797(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-7800(ix), l
	ld	-7799(ix), h
	ld	l, -7798(ix)
	ld	h, -7797(ix)
	push	hl
	ld	l, -7800(ix)
	ld	h, -7799(ix)
	ld	b, l
	pop	hl
__shift_6892:
	ld	a, b
	or	a, a
	jp	z, __sdone_2631
	add	hl, hl
	djnz	__shift_6892
__sdone_2631:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7804(ix), l
	ld	-7803(ix), h
	ld	l, -7804(ix)
	ld	h, -7803(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7806(ix), l
	ld	-7805(ix), h
	ld	l, -7806(ix)
	ld	h, -7805(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39443
	ld	hl, #0
	jp	__cmp_e_4712
__cmp_t_39443:
	ld	hl, #1
__cmp_e_4712:
	dec	sp
	dec	sp
	ld	-7808(ix), l
	ld	-7807(ix), h
	ld	l, -7808(ix)
	ld	h, -7807(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33007
	ld	hl, #0
	jp	__cmp_e_19353
__cmp_t_33007:
	ld	hl, #1
__cmp_e_19353:
	dec	sp
	dec	sp
	ld	-7810(ix), l
	ld	-7809(ix), h
	jp	__xcc_L1641
__xcc_L1639:
	ld	hl, #1
	ld	-7810(ix), l
	ld	-7809(ix), h
__xcc_L1641:
	ld	l, -7810(ix)
	ld	h, -7809(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1636
	jp	__xcc_L1637
__xcc_L1636:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7812(ix), l
	ld	-7811(ix), h
	ld	l, -7812(ix)
	ld	h, -7811(ix)
	dec	sp
	dec	sp
	ld	-7814(ix), l
	ld	-7813(ix), h
	jp	__xcc_L1638
__xcc_L1637:
	ld	hl, #1
	ld	-7814(ix), l
	ld	-7813(ix), h
__xcc_L1638:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7816(ix), l
	ld	-7815(ix), h
	.globl __mul16
	ld	l, -7816(ix)
	ld	h, -7815(ix)
	push	hl
	ld	l, -7814(ix)
	ld	h, -7813(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7818(ix), l
	ld	-7817(ix), h
	ld	l, -7818(ix)
	ld	h, -7817(ix)
	push	hl
	ld	l, -7776(ix)
	ld	h, -7775(ix)
	push	hl
	ld	l, -7746(ix)
	ld	h, -7745(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1614:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1613
	jp	__xcc_L1615
__xcc_L1615:
__xcc_L1642:
	ld	hl, #__str_1645
	dec	sp
	dec	sp
	ld	-7820(ix), l
	ld	-7819(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7822(ix), l
	ld	-7821(ix), h
	ld	l, -7822(ix)
	ld	h, -7821(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7826(ix), l
	ld	-7825(ix), h
	ld	l, -7826(ix)
	ld	h, -7825(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_82313
	ld	hl, #0
	jp	__cmp_e_1662
__cmp_t_82313:
	ld	hl, #1
__cmp_e_1662:
	dec	sp
	dec	sp
	ld	-7828(ix), l
	ld	-7827(ix), h
	ld	l, -7828(ix)
	ld	h, -7827(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_4513
	ld	hl, #0
	jp	__cmp_e_58628
__cmp_t_4513:
	ld	hl, #1
__cmp_e_58628:
	dec	sp
	dec	sp
	ld	-7830(ix), l
	ld	-7829(ix), h
	ld	l, -7830(ix)
	ld	h, -7829(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1649
	jp	__xcc_L1650
__xcc_L1650:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7832(ix), l
	ld	-7831(ix), h
	ld	l, -7832(ix)
	ld	h, -7831(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7836(ix), l
	ld	-7835(ix), h
	ld	l, -7836(ix)
	ld	h, -7835(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7838(ix), l
	ld	-7837(ix), h
	ld	l, -7838(ix)
	ld	h, -7837(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_30096
	ld	hl, #0
	jp	__cmp_e_99551
__cmp_t_30096:
	ld	hl, #1
__cmp_e_99551:
	dec	sp
	dec	sp
	ld	-7840(ix), l
	ld	-7839(ix), h
	ld	l, -7840(ix)
	ld	h, -7839(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_44856
	ld	hl, #0
	jp	__cmp_e_12110
__cmp_t_44856:
	ld	hl, #1
__cmp_e_12110:
	dec	sp
	dec	sp
	ld	-7842(ix), l
	ld	-7841(ix), h
	jp	__xcc_L1651
__xcc_L1649:
	ld	hl, #1
	ld	-7842(ix), l
	ld	-7841(ix), h
__xcc_L1651:
	ld	l, -7842(ix)
	ld	h, -7841(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1646
	jp	__xcc_L1647
__xcc_L1646:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7844(ix), l
	ld	-7843(ix), h
	ld	l, -7844(ix)
	ld	h, -7843(ix)
	dec	sp
	dec	sp
	ld	-7846(ix), l
	ld	-7845(ix), h
	jp	__xcc_L1648
__xcc_L1647:
	ld	hl, #1
	ld	-7846(ix), l
	ld	-7845(ix), h
__xcc_L1648:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7848(ix), l
	ld	-7847(ix), h
	.globl __mul16
	ld	l, -7848(ix)
	ld	h, -7847(ix)
	push	hl
	ld	l, -7846(ix)
	ld	h, -7845(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7850(ix), l
	ld	-7849(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7852(ix), l
	ld	-7851(ix), h
	ld	l, -7852(ix)
	ld	h, -7851(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7856(ix), l
	ld	-7855(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7860(ix), l
	ld	-7859(ix), h
	ld	l, -7856(ix)
	ld	h, -7855(ix)
	push	hl
	ld	l, -7860(ix)
	ld	h, -7859(ix)
	ld	b, l
	pop	hl
__shift_7699:
	ld	a, b
	or	a, a
	jp	z, __sdone_641
	add	hl, hl
	djnz	__shift_7699
__sdone_641:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7864(ix), l
	ld	-7863(ix), h
	ld	l, -7864(ix)
	ld	h, -7863(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3069
	ld	hl, #0
	jp	__cmp_e_52481
__cmp_t_3069:
	ld	hl, #1
__cmp_e_52481:
	dec	sp
	dec	sp
	ld	-7866(ix), l
	ld	-7865(ix), h
	ld	l, -7866(ix)
	ld	h, -7865(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_87195
	ld	hl, #0
	jp	__cmp_e_23090
__cmp_t_87195:
	ld	hl, #1
__cmp_e_23090:
	dec	sp
	dec	sp
	ld	-7868(ix), l
	ld	-7867(ix), h
	ld	l, -7868(ix)
	ld	h, -7867(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1655
	jp	__xcc_L1656
__xcc_L1656:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7870(ix), l
	ld	-7869(ix), h
	ld	l, -7870(ix)
	ld	h, -7869(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7874(ix), l
	ld	-7873(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7878(ix), l
	ld	-7877(ix), h
	ld	l, -7874(ix)
	ld	h, -7873(ix)
	push	hl
	ld	l, -7878(ix)
	ld	h, -7877(ix)
	ld	b, l
	pop	hl
__shift_7889:
	ld	a, b
	or	a, a
	jp	z, __sdone_7854
	add	hl, hl
	djnz	__shift_7889
__sdone_7854:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7882(ix), l
	ld	-7881(ix), h
	ld	l, -7882(ix)
	ld	h, -7881(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7884(ix), l
	ld	-7883(ix), h
	ld	l, -7884(ix)
	ld	h, -7883(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_19369
	ld	hl, #0
	jp	__cmp_e_37736
__cmp_t_19369:
	ld	hl, #1
__cmp_e_37736:
	dec	sp
	dec	sp
	ld	-7886(ix), l
	ld	-7885(ix), h
	ld	l, -7886(ix)
	ld	h, -7885(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_96008
	ld	hl, #0
	jp	__cmp_e_27682
__cmp_t_96008:
	ld	hl, #1
__cmp_e_27682:
	dec	sp
	dec	sp
	ld	-7888(ix), l
	ld	-7887(ix), h
	jp	__xcc_L1657
__xcc_L1655:
	ld	hl, #1
	ld	-7888(ix), l
	ld	-7887(ix), h
__xcc_L1657:
	ld	l, -7888(ix)
	ld	h, -7887(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1652
	jp	__xcc_L1653
__xcc_L1652:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7890(ix), l
	ld	-7889(ix), h
	ld	l, -7890(ix)
	ld	h, -7889(ix)
	dec	sp
	dec	sp
	ld	-7892(ix), l
	ld	-7891(ix), h
	jp	__xcc_L1654
__xcc_L1653:
	ld	hl, #1
	ld	-7892(ix), l
	ld	-7891(ix), h
__xcc_L1654:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7894(ix), l
	ld	-7893(ix), h
	.globl __mul16
	ld	l, -7894(ix)
	ld	h, -7893(ix)
	push	hl
	ld	l, -7892(ix)
	ld	h, -7891(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7896(ix), l
	ld	-7895(ix), h
	ld	l, -7896(ix)
	ld	h, -7895(ix)
	push	hl
	ld	l, -7850(ix)
	ld	h, -7849(ix)
	push	hl
	ld	l, -7820(ix)
	ld	h, -7819(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1658
	dec	sp
	dec	sp
	ld	-7898(ix), l
	ld	-7897(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7900(ix), l
	ld	-7899(ix), h
	ld	l, -7900(ix)
	ld	h, -7899(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7904(ix), l
	ld	-7903(ix), h
	ld	l, -7904(ix)
	ld	h, -7903(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_75704
	ld	hl, #0
	jp	__cmp_e_58726
__cmp_t_75704:
	ld	hl, #1
__cmp_e_58726:
	dec	sp
	dec	sp
	ld	-7906(ix), l
	ld	-7905(ix), h
	ld	l, -7906(ix)
	ld	h, -7905(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34295
	ld	hl, #0
	jp	__cmp_e_35733
__cmp_t_34295:
	ld	hl, #1
__cmp_e_35733:
	dec	sp
	dec	sp
	ld	-7908(ix), l
	ld	-7907(ix), h
	ld	l, -7908(ix)
	ld	h, -7907(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1662
	jp	__xcc_L1663
__xcc_L1663:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7910(ix), l
	ld	-7909(ix), h
	ld	l, -7910(ix)
	ld	h, -7909(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7914(ix), l
	ld	-7913(ix), h
	ld	l, -7914(ix)
	ld	h, -7913(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7916(ix), l
	ld	-7915(ix), h
	ld	l, -7916(ix)
	ld	h, -7915(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_11414
	ld	hl, #0
	jp	__cmp_e_41187
__cmp_t_11414:
	ld	hl, #1
__cmp_e_41187:
	dec	sp
	dec	sp
	ld	-7918(ix), l
	ld	-7917(ix), h
	ld	l, -7918(ix)
	ld	h, -7917(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_28364
	ld	hl, #0
	jp	__cmp_e_50857
__cmp_t_28364:
	ld	hl, #1
__cmp_e_50857:
	dec	sp
	dec	sp
	ld	-7920(ix), l
	ld	-7919(ix), h
	jp	__xcc_L1664
__xcc_L1662:
	ld	hl, #1
	ld	-7920(ix), l
	ld	-7919(ix), h
__xcc_L1664:
	ld	l, -7920(ix)
	ld	h, -7919(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1659
	jp	__xcc_L1660
__xcc_L1659:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7922(ix), l
	ld	-7921(ix), h
	ld	l, -7922(ix)
	ld	h, -7921(ix)
	dec	sp
	dec	sp
	ld	-7924(ix), l
	ld	-7923(ix), h
	jp	__xcc_L1661
__xcc_L1660:
	ld	hl, #1
	ld	-7924(ix), l
	ld	-7923(ix), h
__xcc_L1661:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7926(ix), l
	ld	-7925(ix), h
	.globl __mul16
	ld	l, -7926(ix)
	ld	h, -7925(ix)
	push	hl
	ld	l, -7924(ix)
	ld	h, -7923(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7928(ix), l
	ld	-7927(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7930(ix), l
	ld	-7929(ix), h
	ld	l, -7930(ix)
	ld	h, -7929(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7934(ix), l
	ld	-7933(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7938(ix), l
	ld	-7937(ix), h
	ld	l, -7934(ix)
	ld	h, -7933(ix)
	push	hl
	ld	l, -7938(ix)
	ld	h, -7937(ix)
	ld	b, l
	pop	hl
__shift_2251:
	ld	a, b
	or	a, a
	jp	z, __sdone_7724
	add	hl, hl
	djnz	__shift_2251
__sdone_7724:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7942(ix), l
	ld	-7941(ix), h
	ld	l, -7942(ix)
	ld	h, -7941(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_70210
	ld	hl, #0
	jp	__cmp_e_44564
__cmp_t_70210:
	ld	hl, #1
__cmp_e_44564:
	dec	sp
	dec	sp
	ld	-7944(ix), l
	ld	-7943(ix), h
	ld	l, -7944(ix)
	ld	h, -7943(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_95738
	ld	hl, #0
	jp	__cmp_e_74723
__cmp_t_95738:
	ld	hl, #1
__cmp_e_74723:
	dec	sp
	dec	sp
	ld	-7946(ix), l
	ld	-7945(ix), h
	ld	l, -7946(ix)
	ld	h, -7945(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1668
	jp	__xcc_L1669
__xcc_L1669:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7948(ix), l
	ld	-7947(ix), h
	ld	l, -7948(ix)
	ld	h, -7947(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7952(ix), l
	ld	-7951(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7956(ix), l
	ld	-7955(ix), h
	ld	l, -7952(ix)
	ld	h, -7951(ix)
	push	hl
	ld	l, -7956(ix)
	ld	h, -7955(ix)
	ld	b, l
	pop	hl
__shift_3193:
	ld	a, b
	or	a, a
	jp	z, __sdone_5834
	add	hl, hl
	djnz	__shift_3193
__sdone_5834:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7960(ix), l
	ld	-7959(ix), h
	ld	l, -7960(ix)
	ld	h, -7959(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7962(ix), l
	ld	-7961(ix), h
	ld	l, -7962(ix)
	ld	h, -7961(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_90626
	ld	hl, #0
	jp	__cmp_e_64401
__cmp_t_90626:
	ld	hl, #1
__cmp_e_64401:
	dec	sp
	dec	sp
	ld	-7964(ix), l
	ld	-7963(ix), h
	ld	l, -7964(ix)
	ld	h, -7963(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_37945
	ld	hl, #0
	jp	__cmp_e_18325
__cmp_t_37945:
	ld	hl, #1
__cmp_e_18325:
	dec	sp
	dec	sp
	ld	-7966(ix), l
	ld	-7965(ix), h
	jp	__xcc_L1670
__xcc_L1668:
	ld	hl, #1
	ld	-7966(ix), l
	ld	-7965(ix), h
__xcc_L1670:
	ld	l, -7966(ix)
	ld	h, -7965(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1665
	jp	__xcc_L1666
__xcc_L1665:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7968(ix), l
	ld	-7967(ix), h
	ld	l, -7968(ix)
	ld	h, -7967(ix)
	dec	sp
	dec	sp
	ld	-7970(ix), l
	ld	-7969(ix), h
	jp	__xcc_L1667
__xcc_L1666:
	ld	hl, #1
	ld	-7970(ix), l
	ld	-7969(ix), h
__xcc_L1667:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-7972(ix), l
	ld	-7971(ix), h
	.globl __mul16
	ld	l, -7972(ix)
	ld	h, -7971(ix)
	push	hl
	ld	l, -7970(ix)
	ld	h, -7969(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-7974(ix), l
	ld	-7973(ix), h
	ld	l, -7974(ix)
	ld	h, -7973(ix)
	push	hl
	ld	l, -7928(ix)
	ld	h, -7927(ix)
	push	hl
	ld	l, -7898(ix)
	ld	h, -7897(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1643:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1642
	jp	__xcc_L1644
__xcc_L1644:
__xcc_L1671:
	ld	hl, #__str_1674
	dec	sp
	dec	sp
	ld	-7976(ix), l
	ld	-7975(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7978(ix), l
	ld	-7977(ix), h
	ld	l, -7978(ix)
	ld	h, -7977(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7982(ix), l
	ld	-7981(ix), h
	ld	l, -7982(ix)
	ld	h, -7981(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51394
	ld	hl, #0
	jp	__cmp_e_57366
__cmp_t_51394:
	ld	hl, #1
__cmp_e_57366:
	dec	sp
	dec	sp
	ld	-7984(ix), l
	ld	-7983(ix), h
	ld	l, -7984(ix)
	ld	h, -7983(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70806
	ld	hl, #0
	jp	__cmp_e_38589
__cmp_t_70806:
	ld	hl, #1
__cmp_e_38589:
	dec	sp
	dec	sp
	ld	-7986(ix), l
	ld	-7985(ix), h
	ld	l, -7986(ix)
	ld	h, -7985(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1678
	jp	__xcc_L1679
__xcc_L1679:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7988(ix), l
	ld	-7987(ix), h
	ld	l, -7988(ix)
	ld	h, -7987(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-7992(ix), l
	ld	-7991(ix), h
	ld	l, -7992(ix)
	ld	h, -7991(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-7994(ix), l
	ld	-7993(ix), h
	ld	l, -7994(ix)
	ld	h, -7993(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80456
	ld	hl, #0
	jp	__cmp_e_35047
__cmp_t_80456:
	ld	hl, #1
__cmp_e_35047:
	dec	sp
	dec	sp
	ld	-7996(ix), l
	ld	-7995(ix), h
	ld	l, -7996(ix)
	ld	h, -7995(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_62795
	ld	hl, #0
	jp	__cmp_e_99826
__cmp_t_62795:
	ld	hl, #1
__cmp_e_99826:
	dec	sp
	dec	sp
	ld	-7998(ix), l
	ld	-7997(ix), h
	jp	__xcc_L1680
__xcc_L1678:
	ld	hl, #1
	ld	-7998(ix), l
	ld	-7997(ix), h
__xcc_L1680:
	ld	l, -7998(ix)
	ld	h, -7997(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1675
	jp	__xcc_L1676
__xcc_L1675:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8000(ix), l
	ld	-7999(ix), h
	ld	l, -8000(ix)
	ld	h, -7999(ix)
	dec	sp
	dec	sp
	ld	-8002(ix), l
	ld	-8001(ix), h
	jp	__xcc_L1677
__xcc_L1676:
	ld	hl, #1
	ld	-8002(ix), l
	ld	-8001(ix), h
__xcc_L1677:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-8004(ix), l
	ld	-8003(ix), h
	.globl __mul16
	ld	l, -8004(ix)
	ld	h, -8003(ix)
	push	hl
	ld	l, -8002(ix)
	ld	h, -8001(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8006(ix), l
	ld	-8005(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8008(ix), l
	ld	-8007(ix), h
	ld	l, -8008(ix)
	ld	h, -8007(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8012(ix), l
	ld	-8011(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8020(ix), l
	ld	-8019(ix), h
	ld	l, -8012(ix)
	ld	h, -8011(ix)
	push	hl
	ld	l, -8020(ix)
	ld	h, -8019(ix)
	ld	b, l
	pop	hl
__shift_2784:
	ld	a, b
	or	a, a
	jp	z, __sdone_8803
	add	hl, hl
	djnz	__shift_2784
__sdone_8803:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8028(ix), l
	ld	-8027(ix), h
	ld	l, -8028(ix)
	ld	h, -8027(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_43860
	ld	hl, #0
	jp	__cmp_e_64840
__cmp_t_43860:
	ld	hl, #1
__cmp_e_64840:
	dec	sp
	dec	sp
	ld	-8030(ix), l
	ld	-8029(ix), h
	ld	l, -8030(ix)
	ld	h, -8029(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_33882
	ld	hl, #0
	jp	__cmp_e_78155
__cmp_t_33882:
	ld	hl, #1
__cmp_e_78155:
	dec	sp
	dec	sp
	ld	-8032(ix), l
	ld	-8031(ix), h
	ld	l, -8032(ix)
	ld	h, -8031(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1684
	jp	__xcc_L1685
__xcc_L1685:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8034(ix), l
	ld	-8033(ix), h
	ld	l, -8034(ix)
	ld	h, -8033(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8038(ix), l
	ld	-8037(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8046(ix), l
	ld	-8045(ix), h
	ld	l, -8038(ix)
	ld	h, -8037(ix)
	push	hl
	ld	l, -8046(ix)
	ld	h, -8045(ix)
	ld	b, l
	pop	hl
__shift_573:
	ld	a, b
	or	a, a
	jp	z, __sdone_1648
	add	hl, hl
	djnz	__shift_573
__sdone_1648:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8054(ix), l
	ld	-8053(ix), h
	ld	l, -8054(ix)
	ld	h, -8053(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8056(ix), l
	ld	-8055(ix), h
	ld	l, -8056(ix)
	ld	h, -8055(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35695
	ld	hl, #0
	jp	__cmp_e_45290
__cmp_t_35695:
	ld	hl, #1
__cmp_e_45290:
	dec	sp
	dec	sp
	ld	-8058(ix), l
	ld	-8057(ix), h
	ld	l, -8058(ix)
	ld	h, -8057(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_12505
	ld	hl, #0
	jp	__cmp_e_97946
__cmp_t_12505:
	ld	hl, #1
__cmp_e_97946:
	dec	sp
	dec	sp
	ld	-8060(ix), l
	ld	-8059(ix), h
	jp	__xcc_L1686
__xcc_L1684:
	ld	hl, #1
	ld	-8060(ix), l
	ld	-8059(ix), h
__xcc_L1686:
	ld	l, -8060(ix)
	ld	h, -8059(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1681
	jp	__xcc_L1682
__xcc_L1681:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8062(ix), l
	ld	-8061(ix), h
	ld	l, -8062(ix)
	ld	h, -8061(ix)
	dec	sp
	dec	sp
	ld	-8064(ix), l
	ld	-8063(ix), h
	jp	__xcc_L1683
__xcc_L1682:
	ld	hl, #1
	ld	-8064(ix), l
	ld	-8063(ix), h
__xcc_L1683:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8066(ix), l
	ld	-8065(ix), h
	.globl __mul16
	ld	l, -8066(ix)
	ld	h, -8065(ix)
	push	hl
	ld	l, -8064(ix)
	ld	h, -8063(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8068(ix), l
	ld	-8067(ix), h
	ld	l, -8068(ix)
	ld	h, -8067(ix)
	push	hl
	ld	l, -8006(ix)
	ld	h, -8005(ix)
	push	hl
	ld	l, -7976(ix)
	ld	h, -7975(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1687
	dec	sp
	dec	sp
	ld	-8070(ix), l
	ld	-8069(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8072(ix), l
	ld	-8071(ix), h
	ld	l, -8072(ix)
	ld	h, -8071(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8076(ix), l
	ld	-8075(ix), h
	ld	l, -8076(ix)
	ld	h, -8075(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_39366
	ld	hl, #0
	jp	__cmp_e_99067
__cmp_t_39366:
	ld	hl, #1
__cmp_e_99067:
	dec	sp
	dec	sp
	ld	-8078(ix), l
	ld	-8077(ix), h
	ld	l, -8078(ix)
	ld	h, -8077(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_58863
	ld	hl, #0
	jp	__cmp_e_35104
__cmp_t_58863:
	ld	hl, #1
__cmp_e_35104:
	dec	sp
	dec	sp
	ld	-8080(ix), l
	ld	-8079(ix), h
	ld	l, -8080(ix)
	ld	h, -8079(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1691
	jp	__xcc_L1692
__xcc_L1692:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8082(ix), l
	ld	-8081(ix), h
	ld	l, -8082(ix)
	ld	h, -8081(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8086(ix), l
	ld	-8085(ix), h
	ld	l, -8086(ix)
	ld	h, -8085(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8088(ix), l
	ld	-8087(ix), h
	ld	l, -8088(ix)
	ld	h, -8087(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73790
	ld	hl, #0
	jp	__cmp_e_78408
__cmp_t_73790:
	ld	hl, #1
__cmp_e_78408:
	dec	sp
	dec	sp
	ld	-8090(ix), l
	ld	-8089(ix), h
	ld	l, -8090(ix)
	ld	h, -8089(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_77290
	ld	hl, #0
	jp	__cmp_e_80768
__cmp_t_77290:
	ld	hl, #1
__cmp_e_80768:
	dec	sp
	dec	sp
	ld	-8092(ix), l
	ld	-8091(ix), h
	jp	__xcc_L1693
__xcc_L1691:
	ld	hl, #1
	ld	-8092(ix), l
	ld	-8091(ix), h
__xcc_L1693:
	ld	l, -8092(ix)
	ld	h, -8091(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1688
	jp	__xcc_L1689
__xcc_L1688:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8094(ix), l
	ld	-8093(ix), h
	ld	l, -8094(ix)
	ld	h, -8093(ix)
	dec	sp
	dec	sp
	ld	-8096(ix), l
	ld	-8095(ix), h
	jp	__xcc_L1690
__xcc_L1689:
	ld	hl, #1
	ld	-8096(ix), l
	ld	-8095(ix), h
__xcc_L1690:
	ld	hl, #4
	dec	sp
	dec	sp
	ld	-8098(ix), l
	ld	-8097(ix), h
	.globl __mul16
	ld	l, -8098(ix)
	ld	h, -8097(ix)
	push	hl
	ld	l, -8096(ix)
	ld	h, -8095(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8100(ix), l
	ld	-8099(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8102(ix), l
	ld	-8101(ix), h
	ld	l, -8102(ix)
	ld	h, -8101(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8106(ix), l
	ld	-8105(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8114(ix), l
	ld	-8113(ix), h
	ld	l, -8106(ix)
	ld	h, -8105(ix)
	push	hl
	ld	l, -8114(ix)
	ld	h, -8113(ix)
	ld	b, l
	pop	hl
__shift_9161:
	ld	a, b
	or	a, a
	jp	z, __sdone_5235
	add	hl, hl
	djnz	__shift_9161
__sdone_5235:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8122(ix), l
	ld	-8121(ix), h
	ld	l, -8122(ix)
	ld	h, -8121(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99093
	ld	hl, #0
	jp	__cmp_e_10555
__cmp_t_99093:
	ld	hl, #1
__cmp_e_10555:
	dec	sp
	dec	sp
	ld	-8124(ix), l
	ld	-8123(ix), h
	ld	l, -8124(ix)
	ld	h, -8123(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_72601
	ld	hl, #0
	jp	__cmp_e_86251
__cmp_t_72601:
	ld	hl, #1
__cmp_e_86251:
	dec	sp
	dec	sp
	ld	-8126(ix), l
	ld	-8125(ix), h
	ld	l, -8126(ix)
	ld	h, -8125(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1697
	jp	__xcc_L1698
__xcc_L1698:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8128(ix), l
	ld	-8127(ix), h
	ld	l, -8128(ix)
	ld	h, -8127(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8132(ix), l
	ld	-8131(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8140(ix), l
	ld	-8139(ix), h
	ld	l, -8132(ix)
	ld	h, -8131(ix)
	push	hl
	ld	l, -8140(ix)
	ld	h, -8139(ix)
	ld	b, l
	pop	hl
__shift_9144:
	ld	a, b
	or	a, a
	jp	z, __sdone_9410
	add	hl, hl
	djnz	__shift_9144
__sdone_9410:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8148(ix), l
	ld	-8147(ix), h
	ld	l, -8148(ix)
	ld	h, -8147(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8150(ix), l
	ld	-8149(ix), h
	ld	l, -8150(ix)
	ld	h, -8149(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37651
	ld	hl, #0
	jp	__cmp_e_28291
__cmp_t_37651:
	ld	hl, #1
__cmp_e_28291:
	dec	sp
	dec	sp
	ld	-8152(ix), l
	ld	-8151(ix), h
	ld	l, -8152(ix)
	ld	h, -8151(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85588
	ld	hl, #0
	jp	__cmp_e_10435
__cmp_t_85588:
	ld	hl, #1
__cmp_e_10435:
	dec	sp
	dec	sp
	ld	-8154(ix), l
	ld	-8153(ix), h
	jp	__xcc_L1699
__xcc_L1697:
	ld	hl, #1
	ld	-8154(ix), l
	ld	-8153(ix), h
__xcc_L1699:
	ld	l, -8154(ix)
	ld	h, -8153(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1694
	jp	__xcc_L1695
__xcc_L1694:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8156(ix), l
	ld	-8155(ix), h
	ld	l, -8156(ix)
	ld	h, -8155(ix)
	dec	sp
	dec	sp
	ld	-8158(ix), l
	ld	-8157(ix), h
	jp	__xcc_L1696
__xcc_L1695:
	ld	hl, #1
	ld	-8158(ix), l
	ld	-8157(ix), h
__xcc_L1696:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8160(ix), l
	ld	-8159(ix), h
	.globl __mul16
	ld	l, -8160(ix)
	ld	h, -8159(ix)
	push	hl
	ld	l, -8158(ix)
	ld	h, -8157(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8162(ix), l
	ld	-8161(ix), h
	ld	l, -8162(ix)
	ld	h, -8161(ix)
	push	hl
	ld	l, -8100(ix)
	ld	h, -8099(ix)
	push	hl
	ld	l, -8070(ix)
	ld	h, -8069(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1672:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1671
	jp	__xcc_L1673
__xcc_L1673:
__xcc_L1582:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1581
	jp	__xcc_L1583
__xcc_L1583:
__xcc_L1460:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1459
	jp	__xcc_L1461
__xcc_L1461:
__xcc_L1700:
__xcc_L1703:
__xcc_L1706:
	ld	hl, #__str_1709
	dec	sp
	dec	sp
	ld	-8164(ix), l
	ld	-8163(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8166(ix), l
	ld	-8165(ix), h
	ld	l, -8166(ix)
	ld	h, -8165(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8174(ix), l
	ld	-8173(ix), h
	ld	l, -8174(ix)
	ld	h, -8173(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3447
	ld	hl, #0
	jp	__cmp_e_29448
__cmp_t_3447:
	ld	hl, #1
__cmp_e_29448:
	dec	sp
	dec	sp
	ld	-8176(ix), l
	ld	-8175(ix), h
	ld	l, -8176(ix)
	ld	h, -8175(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_75275
	ld	hl, #0
	jp	__cmp_e_53681
__cmp_t_75275:
	ld	hl, #1
__cmp_e_53681:
	dec	sp
	dec	sp
	ld	-8178(ix), l
	ld	-8177(ix), h
	ld	l, -8178(ix)
	ld	h, -8177(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1713
	jp	__xcc_L1714
__xcc_L1714:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8180(ix), l
	ld	-8179(ix), h
	ld	l, -8180(ix)
	ld	h, -8179(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8188(ix), l
	ld	-8187(ix), h
	ld	l, -8188(ix)
	ld	h, -8187(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8190(ix), l
	ld	-8189(ix), h
	ld	l, -8190(ix)
	ld	h, -8189(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_23956
	ld	hl, #0
	jp	__cmp_e_92200
__cmp_t_23956:
	ld	hl, #1
__cmp_e_92200:
	dec	sp
	dec	sp
	ld	-8192(ix), l
	ld	-8191(ix), h
	ld	l, -8192(ix)
	ld	h, -8191(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15329
	ld	hl, #0
	jp	__cmp_e_59651
__cmp_t_15329:
	ld	hl, #1
__cmp_e_59651:
	dec	sp
	dec	sp
	ld	-8194(ix), l
	ld	-8193(ix), h
	jp	__xcc_L1715
__xcc_L1713:
	ld	hl, #1
	ld	-8194(ix), l
	ld	-8193(ix), h
__xcc_L1715:
	ld	l, -8194(ix)
	ld	h, -8193(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1710
	jp	__xcc_L1711
__xcc_L1710:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8196(ix), l
	ld	-8195(ix), h
	ld	l, -8196(ix)
	ld	h, -8195(ix)
	dec	sp
	dec	sp
	ld	-8198(ix), l
	ld	-8197(ix), h
	jp	__xcc_L1712
__xcc_L1711:
	ld	hl, #1
	ld	-8198(ix), l
	ld	-8197(ix), h
__xcc_L1712:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8200(ix), l
	ld	-8199(ix), h
	.globl __mul16
	ld	l, -8200(ix)
	ld	h, -8199(ix)
	push	hl
	ld	l, -8198(ix)
	ld	h, -8197(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8202(ix), l
	ld	-8201(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8204(ix), l
	ld	-8203(ix), h
	ld	l, -8204(ix)
	ld	h, -8203(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8212(ix), l
	ld	-8211(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8214(ix), l
	ld	-8213(ix), h
	ld	l, -8212(ix)
	ld	h, -8211(ix)
	push	hl
	ld	l, -8214(ix)
	ld	h, -8213(ix)
	ld	b, l
	pop	hl
__shift_3842:
	ld	a, b
	or	a, a
	jp	z, __sdone_7834
	add	hl, hl
	djnz	__shift_3842
__sdone_7834:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8222(ix), l
	ld	-8221(ix), h
	ld	l, -8222(ix)
	ld	h, -8221(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_73949
	ld	hl, #0
	jp	__cmp_e_9560
__cmp_t_73949:
	ld	hl, #1
__cmp_e_9560:
	dec	sp
	dec	sp
	ld	-8224(ix), l
	ld	-8223(ix), h
	ld	l, -8224(ix)
	ld	h, -8223(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_26901
	ld	hl, #0
	jp	__cmp_e_49164
__cmp_t_26901:
	ld	hl, #1
__cmp_e_49164:
	dec	sp
	dec	sp
	ld	-8226(ix), l
	ld	-8225(ix), h
	ld	l, -8226(ix)
	ld	h, -8225(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1719
	jp	__xcc_L1720
__xcc_L1720:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8228(ix), l
	ld	-8227(ix), h
	ld	l, -8228(ix)
	ld	h, -8227(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8236(ix), l
	ld	-8235(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8238(ix), l
	ld	-8237(ix), h
	ld	l, -8236(ix)
	ld	h, -8235(ix)
	push	hl
	ld	l, -8238(ix)
	ld	h, -8237(ix)
	ld	b, l
	pop	hl
__shift_4664:
	ld	a, b
	or	a, a
	jp	z, __sdone_7043
	add	hl, hl
	djnz	__shift_4664
__sdone_7043:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8246(ix), l
	ld	-8245(ix), h
	ld	l, -8246(ix)
	ld	h, -8245(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8248(ix), l
	ld	-8247(ix), h
	ld	l, -8248(ix)
	ld	h, -8247(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_27572
	ld	hl, #0
	jp	__cmp_e_21955
__cmp_t_27572:
	ld	hl, #1
__cmp_e_21955:
	dec	sp
	dec	sp
	ld	-8250(ix), l
	ld	-8249(ix), h
	ld	l, -8250(ix)
	ld	h, -8249(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97811
	ld	hl, #0
	jp	__cmp_e_86733
__cmp_t_97811:
	ld	hl, #1
__cmp_e_86733:
	dec	sp
	dec	sp
	ld	-8252(ix), l
	ld	-8251(ix), h
	jp	__xcc_L1721
__xcc_L1719:
	ld	hl, #1
	ld	-8252(ix), l
	ld	-8251(ix), h
__xcc_L1721:
	ld	l, -8252(ix)
	ld	h, -8251(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1716
	jp	__xcc_L1717
__xcc_L1716:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8254(ix), l
	ld	-8253(ix), h
	ld	l, -8254(ix)
	ld	h, -8253(ix)
	dec	sp
	dec	sp
	ld	-8256(ix), l
	ld	-8255(ix), h
	jp	__xcc_L1718
__xcc_L1717:
	ld	hl, #1
	ld	-8256(ix), l
	ld	-8255(ix), h
__xcc_L1718:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8258(ix), l
	ld	-8257(ix), h
	.globl __mul16
	ld	l, -8258(ix)
	ld	h, -8257(ix)
	push	hl
	ld	l, -8256(ix)
	ld	h, -8255(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8260(ix), l
	ld	-8259(ix), h
	ld	l, -8260(ix)
	ld	h, -8259(ix)
	push	hl
	ld	l, -8202(ix)
	ld	h, -8201(ix)
	push	hl
	ld	l, -8164(ix)
	ld	h, -8163(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1722
	dec	sp
	dec	sp
	ld	-8262(ix), l
	ld	-8261(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8264(ix), l
	ld	-8263(ix), h
	ld	l, -8264(ix)
	ld	h, -8263(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8272(ix), l
	ld	-8271(ix), h
	ld	l, -8272(ix)
	ld	h, -8271(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_53542
	ld	hl, #0
	jp	__cmp_e_13256
__cmp_t_53542:
	ld	hl, #1
__cmp_e_13256:
	dec	sp
	dec	sp
	ld	-8274(ix), l
	ld	-8273(ix), h
	ld	l, -8274(ix)
	ld	h, -8273(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13640
	ld	hl, #0
	jp	__cmp_e_42496
__cmp_t_13640:
	ld	hl, #1
__cmp_e_42496:
	dec	sp
	dec	sp
	ld	-8276(ix), l
	ld	-8275(ix), h
	ld	l, -8276(ix)
	ld	h, -8275(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1726
	jp	__xcc_L1727
__xcc_L1727:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8278(ix), l
	ld	-8277(ix), h
	ld	l, -8278(ix)
	ld	h, -8277(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8286(ix), l
	ld	-8285(ix), h
	ld	l, -8286(ix)
	ld	h, -8285(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8288(ix), l
	ld	-8287(ix), h
	ld	l, -8288(ix)
	ld	h, -8287(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_15859
	ld	hl, #0
	jp	__cmp_e_79136
__cmp_t_15859:
	ld	hl, #1
__cmp_e_79136:
	dec	sp
	dec	sp
	ld	-8290(ix), l
	ld	-8289(ix), h
	ld	l, -8290(ix)
	ld	h, -8289(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_28258
	ld	hl, #0
	jp	__cmp_e_53510
__cmp_t_28258:
	ld	hl, #1
__cmp_e_53510:
	dec	sp
	dec	sp
	ld	-8292(ix), l
	ld	-8291(ix), h
	jp	__xcc_L1728
__xcc_L1726:
	ld	hl, #1
	ld	-8292(ix), l
	ld	-8291(ix), h
__xcc_L1728:
	ld	l, -8292(ix)
	ld	h, -8291(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1723
	jp	__xcc_L1724
__xcc_L1723:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8294(ix), l
	ld	-8293(ix), h
	ld	l, -8294(ix)
	ld	h, -8293(ix)
	dec	sp
	dec	sp
	ld	-8296(ix), l
	ld	-8295(ix), h
	jp	__xcc_L1725
__xcc_L1724:
	ld	hl, #1
	ld	-8296(ix), l
	ld	-8295(ix), h
__xcc_L1725:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8298(ix), l
	ld	-8297(ix), h
	.globl __mul16
	ld	l, -8298(ix)
	ld	h, -8297(ix)
	push	hl
	ld	l, -8296(ix)
	ld	h, -8295(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8300(ix), l
	ld	-8299(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8302(ix), l
	ld	-8301(ix), h
	ld	l, -8302(ix)
	ld	h, -8301(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8310(ix), l
	ld	-8309(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8312(ix), l
	ld	-8311(ix), h
	ld	l, -8310(ix)
	ld	h, -8309(ix)
	push	hl
	ld	l, -8312(ix)
	ld	h, -8311(ix)
	ld	b, l
	pop	hl
__shift_7428:
	ld	a, b
	or	a, a
	jp	z, __sdone_3846
	add	hl, hl
	djnz	__shift_7428
__sdone_3846:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8320(ix), l
	ld	-8319(ix), h
	ld	l, -8320(ix)
	ld	h, -8319(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80297
	ld	hl, #0
	jp	__cmp_e_10875
__cmp_t_80297:
	ld	hl, #1
__cmp_e_10875:
	dec	sp
	dec	sp
	ld	-8322(ix), l
	ld	-8321(ix), h
	ld	l, -8322(ix)
	ld	h, -8321(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43294
	ld	hl, #0
	jp	__cmp_e_71924
__cmp_t_43294:
	ld	hl, #1
__cmp_e_71924:
	dec	sp
	dec	sp
	ld	-8324(ix), l
	ld	-8323(ix), h
	ld	l, -8324(ix)
	ld	h, -8323(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1732
	jp	__xcc_L1733
__xcc_L1733:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8326(ix), l
	ld	-8325(ix), h
	ld	l, -8326(ix)
	ld	h, -8325(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8334(ix), l
	ld	-8333(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8336(ix), l
	ld	-8335(ix), h
	ld	l, -8334(ix)
	ld	h, -8333(ix)
	push	hl
	ld	l, -8336(ix)
	ld	h, -8335(ix)
	ld	b, l
	pop	hl
__shift_4556:
	ld	a, b
	or	a, a
	jp	z, __sdone_7250
	add	hl, hl
	djnz	__shift_4556
__sdone_7250:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8344(ix), l
	ld	-8343(ix), h
	ld	l, -8344(ix)
	ld	h, -8343(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8346(ix), l
	ld	-8345(ix), h
	ld	l, -8346(ix)
	ld	h, -8345(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_64125
	ld	hl, #0
	jp	__cmp_e_79885
__cmp_t_64125:
	ld	hl, #1
__cmp_e_79885:
	dec	sp
	dec	sp
	ld	-8348(ix), l
	ld	-8347(ix), h
	ld	l, -8348(ix)
	ld	h, -8347(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43253
	ld	hl, #0
	jp	__cmp_e_34319
__cmp_t_43253:
	ld	hl, #1
__cmp_e_34319:
	dec	sp
	dec	sp
	ld	-8350(ix), l
	ld	-8349(ix), h
	jp	__xcc_L1734
__xcc_L1732:
	ld	hl, #1
	ld	-8350(ix), l
	ld	-8349(ix), h
__xcc_L1734:
	ld	l, -8350(ix)
	ld	h, -8349(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1729
	jp	__xcc_L1730
__xcc_L1729:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8352(ix), l
	ld	-8351(ix), h
	ld	l, -8352(ix)
	ld	h, -8351(ix)
	dec	sp
	dec	sp
	ld	-8354(ix), l
	ld	-8353(ix), h
	jp	__xcc_L1731
__xcc_L1730:
	ld	hl, #1
	ld	-8354(ix), l
	ld	-8353(ix), h
__xcc_L1731:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8356(ix), l
	ld	-8355(ix), h
	.globl __mul16
	ld	l, -8356(ix)
	ld	h, -8355(ix)
	push	hl
	ld	l, -8354(ix)
	ld	h, -8353(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8358(ix), l
	ld	-8357(ix), h
	ld	l, -8358(ix)
	ld	h, -8357(ix)
	push	hl
	ld	l, -8300(ix)
	ld	h, -8299(ix)
	push	hl
	ld	l, -8262(ix)
	ld	h, -8261(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1707:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1706
	jp	__xcc_L1708
__xcc_L1708:
__xcc_L1735:
	ld	hl, #__str_1738
	dec	sp
	dec	sp
	ld	-8360(ix), l
	ld	-8359(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8362(ix), l
	ld	-8361(ix), h
	ld	l, -8362(ix)
	ld	h, -8361(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8370(ix), l
	ld	-8369(ix), h
	ld	l, -8370(ix)
	ld	h, -8369(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_24071
	ld	hl, #0
	jp	__cmp_e_33555
__cmp_t_24071:
	ld	hl, #1
__cmp_e_33555:
	dec	sp
	dec	sp
	ld	-8372(ix), l
	ld	-8371(ix), h
	ld	l, -8372(ix)
	ld	h, -8371(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_43880
	ld	hl, #0
	jp	__cmp_e_67324
__cmp_t_43880:
	ld	hl, #1
__cmp_e_67324:
	dec	sp
	dec	sp
	ld	-8374(ix), l
	ld	-8373(ix), h
	ld	l, -8374(ix)
	ld	h, -8373(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1742
	jp	__xcc_L1743
__xcc_L1743:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8376(ix), l
	ld	-8375(ix), h
	ld	l, -8376(ix)
	ld	h, -8375(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8384(ix), l
	ld	-8383(ix), h
	ld	l, -8384(ix)
	ld	h, -8383(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8386(ix), l
	ld	-8385(ix), h
	ld	l, -8386(ix)
	ld	h, -8385(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_82719
	ld	hl, #0
	jp	__cmp_e_4896
__cmp_t_82719:
	ld	hl, #1
__cmp_e_4896:
	dec	sp
	dec	sp
	ld	-8388(ix), l
	ld	-8387(ix), h
	ld	l, -8388(ix)
	ld	h, -8387(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_84367
	ld	hl, #0
	jp	__cmp_e_26644
__cmp_t_84367:
	ld	hl, #1
__cmp_e_26644:
	dec	sp
	dec	sp
	ld	-8390(ix), l
	ld	-8389(ix), h
	jp	__xcc_L1744
__xcc_L1742:
	ld	hl, #1
	ld	-8390(ix), l
	ld	-8389(ix), h
__xcc_L1744:
	ld	l, -8390(ix)
	ld	h, -8389(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1739
	jp	__xcc_L1740
__xcc_L1739:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8392(ix), l
	ld	-8391(ix), h
	ld	l, -8392(ix)
	ld	h, -8391(ix)
	dec	sp
	dec	sp
	ld	-8394(ix), l
	ld	-8393(ix), h
	jp	__xcc_L1741
__xcc_L1740:
	ld	hl, #1
	ld	-8394(ix), l
	ld	-8393(ix), h
__xcc_L1741:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8396(ix), l
	ld	-8395(ix), h
	.globl __mul16
	ld	l, -8396(ix)
	ld	h, -8395(ix)
	push	hl
	ld	l, -8394(ix)
	ld	h, -8393(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8398(ix), l
	ld	-8397(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8400(ix), l
	ld	-8399(ix), h
	ld	l, -8400(ix)
	ld	h, -8399(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8408(ix), l
	ld	-8407(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8410(ix), l
	ld	-8409(ix), h
	ld	l, -8408(ix)
	ld	h, -8407(ix)
	push	hl
	ld	l, -8410(ix)
	ld	h, -8409(ix)
	ld	b, l
	pop	hl
__shift_3203:
	ld	a, b
	or	a, a
	jp	z, __sdone_8530
	add	hl, hl
	djnz	__shift_3203
__sdone_8530:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8418(ix), l
	ld	-8417(ix), h
	ld	l, -8418(ix)
	ld	h, -8417(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_29729
	ld	hl, #0
	jp	__cmp_e_96746
__cmp_t_29729:
	ld	hl, #1
__cmp_e_96746:
	dec	sp
	dec	sp
	ld	-8420(ix), l
	ld	-8419(ix), h
	ld	l, -8420(ix)
	ld	h, -8419(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_11786
	ld	hl, #0
	jp	__cmp_e_43370
__cmp_t_11786:
	ld	hl, #1
__cmp_e_43370:
	dec	sp
	dec	sp
	ld	-8422(ix), l
	ld	-8421(ix), h
	ld	l, -8422(ix)
	ld	h, -8421(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1748
	jp	__xcc_L1749
__xcc_L1749:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8424(ix), l
	ld	-8423(ix), h
	ld	l, -8424(ix)
	ld	h, -8423(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8432(ix), l
	ld	-8431(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8434(ix), l
	ld	-8433(ix), h
	ld	l, -8432(ix)
	ld	h, -8431(ix)
	push	hl
	ld	l, -8434(ix)
	ld	h, -8433(ix)
	ld	b, l
	pop	hl
__shift_5594:
	ld	a, b
	or	a, a
	jp	z, __sdone_7645
	add	hl, hl
	djnz	__shift_5594
__sdone_7645:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8442(ix), l
	ld	-8441(ix), h
	ld	l, -8442(ix)
	ld	h, -8441(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8444(ix), l
	ld	-8443(ix), h
	ld	l, -8444(ix)
	ld	h, -8443(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_38858
	ld	hl, #0
	jp	__cmp_e_83852
__cmp_t_38858:
	ld	hl, #1
__cmp_e_83852:
	dec	sp
	dec	sp
	ld	-8446(ix), l
	ld	-8445(ix), h
	ld	l, -8446(ix)
	ld	h, -8445(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97508
	ld	hl, #0
	jp	__cmp_e_46286
__cmp_t_97508:
	ld	hl, #1
__cmp_e_46286:
	dec	sp
	dec	sp
	ld	-8448(ix), l
	ld	-8447(ix), h
	jp	__xcc_L1750
__xcc_L1748:
	ld	hl, #1
	ld	-8448(ix), l
	ld	-8447(ix), h
__xcc_L1750:
	ld	l, -8448(ix)
	ld	h, -8447(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1745
	jp	__xcc_L1746
__xcc_L1745:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8450(ix), l
	ld	-8449(ix), h
	ld	l, -8450(ix)
	ld	h, -8449(ix)
	dec	sp
	dec	sp
	ld	-8452(ix), l
	ld	-8451(ix), h
	jp	__xcc_L1747
__xcc_L1746:
	ld	hl, #1
	ld	-8452(ix), l
	ld	-8451(ix), h
__xcc_L1747:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8454(ix), l
	ld	-8453(ix), h
	.globl __mul16
	ld	l, -8454(ix)
	ld	h, -8453(ix)
	push	hl
	ld	l, -8452(ix)
	ld	h, -8451(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8456(ix), l
	ld	-8455(ix), h
	ld	l, -8456(ix)
	ld	h, -8455(ix)
	push	hl
	ld	l, -8398(ix)
	ld	h, -8397(ix)
	push	hl
	ld	l, -8360(ix)
	ld	h, -8359(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1751
	dec	sp
	dec	sp
	ld	-8458(ix), l
	ld	-8457(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8460(ix), l
	ld	-8459(ix), h
	ld	l, -8460(ix)
	ld	h, -8459(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8468(ix), l
	ld	-8467(ix), h
	ld	l, -8468(ix)
	ld	h, -8467(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_97698
	ld	hl, #0
	jp	__cmp_e_77805
__cmp_t_97698:
	ld	hl, #1
__cmp_e_77805:
	dec	sp
	dec	sp
	ld	-8470(ix), l
	ld	-8469(ix), h
	ld	l, -8470(ix)
	ld	h, -8469(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_73513
	ld	hl, #0
	jp	__cmp_e_57344
__cmp_t_73513:
	ld	hl, #1
__cmp_e_57344:
	dec	sp
	dec	sp
	ld	-8472(ix), l
	ld	-8471(ix), h
	ld	l, -8472(ix)
	ld	h, -8471(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1755
	jp	__xcc_L1756
__xcc_L1756:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8474(ix), l
	ld	-8473(ix), h
	ld	l, -8474(ix)
	ld	h, -8473(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8482(ix), l
	ld	-8481(ix), h
	ld	l, -8482(ix)
	ld	h, -8481(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8484(ix), l
	ld	-8483(ix), h
	ld	l, -8484(ix)
	ld	h, -8483(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_49730
	ld	hl, #0
	jp	__cmp_e_54421
__cmp_t_49730:
	ld	hl, #1
__cmp_e_54421:
	dec	sp
	dec	sp
	ld	-8486(ix), l
	ld	-8485(ix), h
	ld	l, -8486(ix)
	ld	h, -8485(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_40947
	ld	hl, #0
	jp	__cmp_e_30207
__cmp_t_40947:
	ld	hl, #1
__cmp_e_30207:
	dec	sp
	dec	sp
	ld	-8488(ix), l
	ld	-8487(ix), h
	jp	__xcc_L1757
__xcc_L1755:
	ld	hl, #1
	ld	-8488(ix), l
	ld	-8487(ix), h
__xcc_L1757:
	ld	l, -8488(ix)
	ld	h, -8487(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1752
	jp	__xcc_L1753
__xcc_L1752:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8490(ix), l
	ld	-8489(ix), h
	ld	l, -8490(ix)
	ld	h, -8489(ix)
	dec	sp
	dec	sp
	ld	-8492(ix), l
	ld	-8491(ix), h
	jp	__xcc_L1754
__xcc_L1753:
	ld	hl, #1
	ld	-8492(ix), l
	ld	-8491(ix), h
__xcc_L1754:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8494(ix), l
	ld	-8493(ix), h
	.globl __mul16
	ld	l, -8494(ix)
	ld	h, -8493(ix)
	push	hl
	ld	l, -8492(ix)
	ld	h, -8491(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8496(ix), l
	ld	-8495(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8498(ix), l
	ld	-8497(ix), h
	ld	l, -8498(ix)
	ld	h, -8497(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8506(ix), l
	ld	-8505(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8508(ix), l
	ld	-8507(ix), h
	ld	l, -8506(ix)
	ld	h, -8505(ix)
	push	hl
	ld	l, -8508(ix)
	ld	h, -8507(ix)
	ld	b, l
	pop	hl
__shift_658:
	ld	a, b
	or	a, a
	jp	z, __sdone_4200
	add	hl, hl
	djnz	__shift_658
__sdone_4200:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8516(ix), l
	ld	-8515(ix), h
	ld	l, -8516(ix)
	ld	h, -8515(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_80878
	ld	hl, #0
	jp	__cmp_e_74729
__cmp_t_80878:
	ld	hl, #1
__cmp_e_74729:
	dec	sp
	dec	sp
	ld	-8518(ix), l
	ld	-8517(ix), h
	ld	l, -8518(ix)
	ld	h, -8517(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34107
	ld	hl, #0
	jp	__cmp_e_24758
__cmp_t_34107:
	ld	hl, #1
__cmp_e_24758:
	dec	sp
	dec	sp
	ld	-8520(ix), l
	ld	-8519(ix), h
	ld	l, -8520(ix)
	ld	h, -8519(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1761
	jp	__xcc_L1762
__xcc_L1762:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8522(ix), l
	ld	-8521(ix), h
	ld	l, -8522(ix)
	ld	h, -8521(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8530(ix), l
	ld	-8529(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-8532(ix), l
	ld	-8531(ix), h
	ld	l, -8530(ix)
	ld	h, -8529(ix)
	push	hl
	ld	l, -8532(ix)
	ld	h, -8531(ix)
	ld	b, l
	pop	hl
__shift_2053:
	ld	a, b
	or	a, a
	jp	z, __sdone_3179
	add	hl, hl
	djnz	__shift_2053
__sdone_3179:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8540(ix), l
	ld	-8539(ix), h
	ld	l, -8540(ix)
	ld	h, -8539(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8542(ix), l
	ld	-8541(ix), h
	ld	l, -8542(ix)
	ld	h, -8541(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_46007
	ld	hl, #0
	jp	__cmp_e_42772
__cmp_t_46007:
	ld	hl, #1
__cmp_e_42772:
	dec	sp
	dec	sp
	ld	-8544(ix), l
	ld	-8543(ix), h
	ld	l, -8544(ix)
	ld	h, -8543(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_76175
	ld	hl, #0
	jp	__cmp_e_89210
__cmp_t_76175:
	ld	hl, #1
__cmp_e_89210:
	dec	sp
	dec	sp
	ld	-8546(ix), l
	ld	-8545(ix), h
	jp	__xcc_L1763
__xcc_L1761:
	ld	hl, #1
	ld	-8546(ix), l
	ld	-8545(ix), h
__xcc_L1763:
	ld	l, -8546(ix)
	ld	h, -8545(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1758
	jp	__xcc_L1759
__xcc_L1758:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8548(ix), l
	ld	-8547(ix), h
	ld	l, -8548(ix)
	ld	h, -8547(ix)
	dec	sp
	dec	sp
	ld	-8550(ix), l
	ld	-8549(ix), h
	jp	__xcc_L1760
__xcc_L1759:
	ld	hl, #1
	ld	-8550(ix), l
	ld	-8549(ix), h
__xcc_L1760:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8552(ix), l
	ld	-8551(ix), h
	.globl __mul16
	ld	l, -8552(ix)
	ld	h, -8551(ix)
	push	hl
	ld	l, -8550(ix)
	ld	h, -8549(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8554(ix), l
	ld	-8553(ix), h
	ld	l, -8554(ix)
	ld	h, -8553(ix)
	push	hl
	ld	l, -8496(ix)
	ld	h, -8495(ix)
	push	hl
	ld	l, -8458(ix)
	ld	h, -8457(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1736:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1735
	jp	__xcc_L1737
__xcc_L1737:
__xcc_L1764:
	ld	hl, #__str_1767
	dec	sp
	dec	sp
	ld	-8556(ix), l
	ld	-8555(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8558(ix), l
	ld	-8557(ix), h
	ld	l, -8558(ix)
	ld	h, -8557(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8566(ix), l
	ld	-8565(ix), h
	ld	l, -8566(ix)
	ld	h, -8565(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_41302
	ld	hl, #0
	jp	__cmp_e_5904
__cmp_t_41302:
	ld	hl, #1
__cmp_e_5904:
	dec	sp
	dec	sp
	ld	-8568(ix), l
	ld	-8567(ix), h
	ld	l, -8568(ix)
	ld	h, -8567(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2308
	ld	hl, #0
	jp	__cmp_e_69440
__cmp_t_2308:
	ld	hl, #1
__cmp_e_69440:
	dec	sp
	dec	sp
	ld	-8570(ix), l
	ld	-8569(ix), h
	ld	l, -8570(ix)
	ld	h, -8569(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1771
	jp	__xcc_L1772
__xcc_L1772:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8572(ix), l
	ld	-8571(ix), h
	ld	l, -8572(ix)
	ld	h, -8571(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8580(ix), l
	ld	-8579(ix), h
	ld	l, -8580(ix)
	ld	h, -8579(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8582(ix), l
	ld	-8581(ix), h
	ld	l, -8582(ix)
	ld	h, -8581(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_65626
	ld	hl, #0
	jp	__cmp_e_57902
__cmp_t_65626:
	ld	hl, #1
__cmp_e_57902:
	dec	sp
	dec	sp
	ld	-8584(ix), l
	ld	-8583(ix), h
	ld	l, -8584(ix)
	ld	h, -8583(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97086
	ld	hl, #0
	jp	__cmp_e_4485
__cmp_t_97086:
	ld	hl, #1
__cmp_e_4485:
	dec	sp
	dec	sp
	ld	-8586(ix), l
	ld	-8585(ix), h
	jp	__xcc_L1773
__xcc_L1771:
	ld	hl, #1
	ld	-8586(ix), l
	ld	-8585(ix), h
__xcc_L1773:
	ld	l, -8586(ix)
	ld	h, -8585(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1768
	jp	__xcc_L1769
__xcc_L1768:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8588(ix), l
	ld	-8587(ix), h
	ld	l, -8588(ix)
	ld	h, -8587(ix)
	dec	sp
	dec	sp
	ld	-8590(ix), l
	ld	-8589(ix), h
	jp	__xcc_L1770
__xcc_L1769:
	ld	hl, #1
	ld	-8590(ix), l
	ld	-8589(ix), h
__xcc_L1770:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8592(ix), l
	ld	-8591(ix), h
	.globl __mul16
	ld	l, -8592(ix)
	ld	h, -8591(ix)
	push	hl
	ld	l, -8590(ix)
	ld	h, -8589(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8594(ix), l
	ld	-8593(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8596(ix), l
	ld	-8595(ix), h
	ld	l, -8596(ix)
	ld	h, -8595(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8604(ix), l
	ld	-8603(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8608(ix), l
	ld	-8607(ix), h
	ld	l, -8604(ix)
	ld	h, -8603(ix)
	push	hl
	ld	l, -8608(ix)
	ld	h, -8607(ix)
	ld	b, l
	pop	hl
__shift_8106:
	ld	a, b
	or	a, a
	jp	z, __sdone_946
	add	hl, hl
	djnz	__shift_8106
__sdone_946:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8616(ix), l
	ld	-8615(ix), h
	ld	l, -8616(ix)
	ld	h, -8615(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_67123
	ld	hl, #0
	jp	__cmp_e_55804
__cmp_t_67123:
	ld	hl, #1
__cmp_e_55804:
	dec	sp
	dec	sp
	ld	-8618(ix), l
	ld	-8617(ix), h
	ld	l, -8618(ix)
	ld	h, -8617(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88751
	ld	hl, #0
	jp	__cmp_e_40637
__cmp_t_88751:
	ld	hl, #1
__cmp_e_40637:
	dec	sp
	dec	sp
	ld	-8620(ix), l
	ld	-8619(ix), h
	ld	l, -8620(ix)
	ld	h, -8619(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1777
	jp	__xcc_L1778
__xcc_L1778:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8622(ix), l
	ld	-8621(ix), h
	ld	l, -8622(ix)
	ld	h, -8621(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8630(ix), l
	ld	-8629(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8634(ix), l
	ld	-8633(ix), h
	ld	l, -8630(ix)
	ld	h, -8629(ix)
	push	hl
	ld	l, -8634(ix)
	ld	h, -8633(ix)
	ld	b, l
	pop	hl
__shift_9501:
	ld	a, b
	or	a, a
	jp	z, __sdone_4833
	add	hl, hl
	djnz	__shift_9501
__sdone_4833:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8642(ix), l
	ld	-8641(ix), h
	ld	l, -8642(ix)
	ld	h, -8641(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8644(ix), l
	ld	-8643(ix), h
	ld	l, -8644(ix)
	ld	h, -8643(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_11410
	ld	hl, #0
	jp	__cmp_e_70448
__cmp_t_11410:
	ld	hl, #1
__cmp_e_70448:
	dec	sp
	dec	sp
	ld	-8646(ix), l
	ld	-8645(ix), h
	ld	l, -8646(ix)
	ld	h, -8645(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_1392
	ld	hl, #0
	jp	__cmp_e_62069
__cmp_t_1392:
	ld	hl, #1
__cmp_e_62069:
	dec	sp
	dec	sp
	ld	-8648(ix), l
	ld	-8647(ix), h
	jp	__xcc_L1779
__xcc_L1777:
	ld	hl, #1
	ld	-8648(ix), l
	ld	-8647(ix), h
__xcc_L1779:
	ld	l, -8648(ix)
	ld	h, -8647(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1774
	jp	__xcc_L1775
__xcc_L1774:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8650(ix), l
	ld	-8649(ix), h
	ld	l, -8650(ix)
	ld	h, -8649(ix)
	dec	sp
	dec	sp
	ld	-8652(ix), l
	ld	-8651(ix), h
	jp	__xcc_L1776
__xcc_L1775:
	ld	hl, #1
	ld	-8652(ix), l
	ld	-8651(ix), h
__xcc_L1776:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8654(ix), l
	ld	-8653(ix), h
	.globl __mul16
	ld	l, -8654(ix)
	ld	h, -8653(ix)
	push	hl
	ld	l, -8652(ix)
	ld	h, -8651(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8656(ix), l
	ld	-8655(ix), h
	ld	l, -8656(ix)
	ld	h, -8655(ix)
	push	hl
	ld	l, -8594(ix)
	ld	h, -8593(ix)
	push	hl
	ld	l, -8556(ix)
	ld	h, -8555(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1780
	dec	sp
	dec	sp
	ld	-8658(ix), l
	ld	-8657(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8660(ix), l
	ld	-8659(ix), h
	ld	l, -8660(ix)
	ld	h, -8659(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8668(ix), l
	ld	-8667(ix), h
	ld	l, -8668(ix)
	ld	h, -8667(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_71000
	ld	hl, #0
	jp	__cmp_e_82271
__cmp_t_71000:
	ld	hl, #1
__cmp_e_82271:
	dec	sp
	dec	sp
	ld	-8670(ix), l
	ld	-8669(ix), h
	ld	l, -8670(ix)
	ld	h, -8669(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_53150
	ld	hl, #0
	jp	__cmp_e_5108
__cmp_t_53150:
	ld	hl, #1
__cmp_e_5108:
	dec	sp
	dec	sp
	ld	-8672(ix), l
	ld	-8671(ix), h
	ld	l, -8672(ix)
	ld	h, -8671(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1784
	jp	__xcc_L1785
__xcc_L1785:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8674(ix), l
	ld	-8673(ix), h
	ld	l, -8674(ix)
	ld	h, -8673(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8682(ix), l
	ld	-8681(ix), h
	ld	l, -8682(ix)
	ld	h, -8681(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8684(ix), l
	ld	-8683(ix), h
	ld	l, -8684(ix)
	ld	h, -8683(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_23381
	ld	hl, #0
	jp	__cmp_e_11556
__cmp_t_23381:
	ld	hl, #1
__cmp_e_11556:
	dec	sp
	dec	sp
	ld	-8686(ix), l
	ld	-8685(ix), h
	ld	l, -8686(ix)
	ld	h, -8685(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_38287
	ld	hl, #0
	jp	__cmp_e_69388
__cmp_t_38287:
	ld	hl, #1
__cmp_e_69388:
	dec	sp
	dec	sp
	ld	-8688(ix), l
	ld	-8687(ix), h
	jp	__xcc_L1786
__xcc_L1784:
	ld	hl, #1
	ld	-8688(ix), l
	ld	-8687(ix), h
__xcc_L1786:
	ld	l, -8688(ix)
	ld	h, -8687(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1781
	jp	__xcc_L1782
__xcc_L1781:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8690(ix), l
	ld	-8689(ix), h
	ld	l, -8690(ix)
	ld	h, -8689(ix)
	dec	sp
	dec	sp
	ld	-8692(ix), l
	ld	-8691(ix), h
	jp	__xcc_L1783
__xcc_L1782:
	ld	hl, #1
	ld	-8692(ix), l
	ld	-8691(ix), h
__xcc_L1783:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8694(ix), l
	ld	-8693(ix), h
	.globl __mul16
	ld	l, -8694(ix)
	ld	h, -8693(ix)
	push	hl
	ld	l, -8692(ix)
	ld	h, -8691(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8696(ix), l
	ld	-8695(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8698(ix), l
	ld	-8697(ix), h
	ld	l, -8698(ix)
	ld	h, -8697(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8706(ix), l
	ld	-8705(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8710(ix), l
	ld	-8709(ix), h
	ld	l, -8706(ix)
	ld	h, -8705(ix)
	push	hl
	ld	l, -8710(ix)
	ld	h, -8709(ix)
	ld	b, l
	pop	hl
__shift_4328:
	ld	a, b
	or	a, a
	jp	z, __sdone_4462
	add	hl, hl
	djnz	__shift_4328
__sdone_4462:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8718(ix), l
	ld	-8717(ix), h
	ld	l, -8718(ix)
	ld	h, -8717(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_74951
	ld	hl, #0
	jp	__cmp_e_11983
__cmp_t_74951:
	ld	hl, #1
__cmp_e_11983:
	dec	sp
	dec	sp
	ld	-8720(ix), l
	ld	-8719(ix), h
	ld	l, -8720(ix)
	ld	h, -8719(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_36718
	ld	hl, #0
	jp	__cmp_e_77259
__cmp_t_36718:
	ld	hl, #1
__cmp_e_77259:
	dec	sp
	dec	sp
	ld	-8722(ix), l
	ld	-8721(ix), h
	ld	l, -8722(ix)
	ld	h, -8721(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1790
	jp	__xcc_L1791
__xcc_L1791:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8724(ix), l
	ld	-8723(ix), h
	ld	l, -8724(ix)
	ld	h, -8723(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8732(ix), l
	ld	-8731(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8736(ix), l
	ld	-8735(ix), h
	ld	l, -8732(ix)
	ld	h, -8731(ix)
	push	hl
	ld	l, -8736(ix)
	ld	h, -8735(ix)
	ld	b, l
	pop	hl
__shift_1423:
	ld	a, b
	or	a, a
	jp	z, __sdone_2345
	add	hl, hl
	djnz	__shift_1423
__sdone_2345:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8744(ix), l
	ld	-8743(ix), h
	ld	l, -8744(ix)
	ld	h, -8743(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8746(ix), l
	ld	-8745(ix), h
	ld	l, -8746(ix)
	ld	h, -8745(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_51514
	ld	hl, #0
	jp	__cmp_e_94861
__cmp_t_51514:
	ld	hl, #1
__cmp_e_94861:
	dec	sp
	dec	sp
	ld	-8748(ix), l
	ld	-8747(ix), h
	ld	l, -8748(ix)
	ld	h, -8747(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_23182
	ld	hl, #0
	jp	__cmp_e_25972
__cmp_t_23182:
	ld	hl, #1
__cmp_e_25972:
	dec	sp
	dec	sp
	ld	-8750(ix), l
	ld	-8749(ix), h
	jp	__xcc_L1792
__xcc_L1790:
	ld	hl, #1
	ld	-8750(ix), l
	ld	-8749(ix), h
__xcc_L1792:
	ld	l, -8750(ix)
	ld	h, -8749(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1787
	jp	__xcc_L1788
__xcc_L1787:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8752(ix), l
	ld	-8751(ix), h
	ld	l, -8752(ix)
	ld	h, -8751(ix)
	dec	sp
	dec	sp
	ld	-8754(ix), l
	ld	-8753(ix), h
	jp	__xcc_L1789
__xcc_L1788:
	ld	hl, #1
	ld	-8754(ix), l
	ld	-8753(ix), h
__xcc_L1789:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8756(ix), l
	ld	-8755(ix), h
	.globl __mul16
	ld	l, -8756(ix)
	ld	h, -8755(ix)
	push	hl
	ld	l, -8754(ix)
	ld	h, -8753(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8758(ix), l
	ld	-8757(ix), h
	ld	l, -8758(ix)
	ld	h, -8757(ix)
	push	hl
	ld	l, -8696(ix)
	ld	h, -8695(ix)
	push	hl
	ld	l, -8658(ix)
	ld	h, -8657(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1765:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1764
	jp	__xcc_L1766
__xcc_L1766:
__xcc_L1793:
	ld	hl, #__str_1796
	dec	sp
	dec	sp
	ld	-8760(ix), l
	ld	-8759(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8762(ix), l
	ld	-8761(ix), h
	ld	l, -8762(ix)
	ld	h, -8761(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8770(ix), l
	ld	-8769(ix), h
	ld	l, -8770(ix)
	ld	h, -8769(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_5807
	ld	hl, #0
	jp	__cmp_e_6657
__cmp_t_5807:
	ld	hl, #1
__cmp_e_6657:
	dec	sp
	dec	sp
	ld	-8772(ix), l
	ld	-8771(ix), h
	ld	l, -8772(ix)
	ld	h, -8771(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_98129
	ld	hl, #0
	jp	__cmp_e_10911
__cmp_t_98129:
	ld	hl, #1
__cmp_e_10911:
	dec	sp
	dec	sp
	ld	-8774(ix), l
	ld	-8773(ix), h
	ld	l, -8774(ix)
	ld	h, -8773(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1800
	jp	__xcc_L1801
__xcc_L1801:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8776(ix), l
	ld	-8775(ix), h
	ld	l, -8776(ix)
	ld	h, -8775(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8784(ix), l
	ld	-8783(ix), h
	ld	l, -8784(ix)
	ld	h, -8783(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8786(ix), l
	ld	-8785(ix), h
	ld	l, -8786(ix)
	ld	h, -8785(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_47294
	ld	hl, #0
	jp	__cmp_e_27630
__cmp_t_47294:
	ld	hl, #1
__cmp_e_27630:
	dec	sp
	dec	sp
	ld	-8788(ix), l
	ld	-8787(ix), h
	ld	l, -8788(ix)
	ld	h, -8787(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65744
	ld	hl, #0
	jp	__cmp_e_75057
__cmp_t_65744:
	ld	hl, #1
__cmp_e_75057:
	dec	sp
	dec	sp
	ld	-8790(ix), l
	ld	-8789(ix), h
	jp	__xcc_L1802
__xcc_L1800:
	ld	hl, #1
	ld	-8790(ix), l
	ld	-8789(ix), h
__xcc_L1802:
	ld	l, -8790(ix)
	ld	h, -8789(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1797
	jp	__xcc_L1798
__xcc_L1797:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8792(ix), l
	ld	-8791(ix), h
	ld	l, -8792(ix)
	ld	h, -8791(ix)
	dec	sp
	dec	sp
	ld	-8794(ix), l
	ld	-8793(ix), h
	jp	__xcc_L1799
__xcc_L1798:
	ld	hl, #1
	ld	-8794(ix), l
	ld	-8793(ix), h
__xcc_L1799:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8796(ix), l
	ld	-8795(ix), h
	.globl __mul16
	ld	l, -8796(ix)
	ld	h, -8795(ix)
	push	hl
	ld	l, -8794(ix)
	ld	h, -8793(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8798(ix), l
	ld	-8797(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8800(ix), l
	ld	-8799(ix), h
	ld	l, -8800(ix)
	ld	h, -8799(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8808(ix), l
	ld	-8807(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8816(ix), l
	ld	-8815(ix), h
	ld	l, -8808(ix)
	ld	h, -8807(ix)
	push	hl
	ld	l, -8816(ix)
	ld	h, -8815(ix)
	ld	b, l
	pop	hl
__shift_8078:
	ld	a, b
	or	a, a
	jp	z, __sdone_7137
	add	hl, hl
	djnz	__shift_8078
__sdone_7137:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8824(ix), l
	ld	-8823(ix), h
	ld	l, -8824(ix)
	ld	h, -8823(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37126
	ld	hl, #0
	jp	__cmp_e_69078
__cmp_t_37126:
	ld	hl, #1
__cmp_e_69078:
	dec	sp
	dec	sp
	ld	-8826(ix), l
	ld	-8825(ix), h
	ld	l, -8826(ix)
	ld	h, -8825(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_65760
	ld	hl, #0
	jp	__cmp_e_6628
__cmp_t_65760:
	ld	hl, #1
__cmp_e_6628:
	dec	sp
	dec	sp
	ld	-8828(ix), l
	ld	-8827(ix), h
	ld	l, -8828(ix)
	ld	h, -8827(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1806
	jp	__xcc_L1807
__xcc_L1807:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8830(ix), l
	ld	-8829(ix), h
	ld	l, -8830(ix)
	ld	h, -8829(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8838(ix), l
	ld	-8837(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8846(ix), l
	ld	-8845(ix), h
	ld	l, -8838(ix)
	ld	h, -8837(ix)
	push	hl
	ld	l, -8846(ix)
	ld	h, -8845(ix)
	ld	b, l
	pop	hl
__shift_4186:
	ld	a, b
	or	a, a
	jp	z, __sdone_9141
	add	hl, hl
	djnz	__shift_4186
__sdone_9141:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8854(ix), l
	ld	-8853(ix), h
	ld	l, -8854(ix)
	ld	h, -8853(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8856(ix), l
	ld	-8855(ix), h
	ld	l, -8856(ix)
	ld	h, -8855(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_18184
	ld	hl, #0
	jp	__cmp_e_28825
__cmp_t_18184:
	ld	hl, #1
__cmp_e_28825:
	dec	sp
	dec	sp
	ld	-8858(ix), l
	ld	-8857(ix), h
	ld	l, -8858(ix)
	ld	h, -8857(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_74882
	ld	hl, #0
	jp	__cmp_e_88865
__cmp_t_74882:
	ld	hl, #1
__cmp_e_88865:
	dec	sp
	dec	sp
	ld	-8860(ix), l
	ld	-8859(ix), h
	jp	__xcc_L1808
__xcc_L1806:
	ld	hl, #1
	ld	-8860(ix), l
	ld	-8859(ix), h
__xcc_L1808:
	ld	l, -8860(ix)
	ld	h, -8859(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1803
	jp	__xcc_L1804
__xcc_L1803:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8862(ix), l
	ld	-8861(ix), h
	ld	l, -8862(ix)
	ld	h, -8861(ix)
	dec	sp
	dec	sp
	ld	-8864(ix), l
	ld	-8863(ix), h
	jp	__xcc_L1805
__xcc_L1804:
	ld	hl, #1
	ld	-8864(ix), l
	ld	-8863(ix), h
__xcc_L1805:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8866(ix), l
	ld	-8865(ix), h
	.globl __mul16
	ld	l, -8866(ix)
	ld	h, -8865(ix)
	push	hl
	ld	l, -8864(ix)
	ld	h, -8863(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8868(ix), l
	ld	-8867(ix), h
	ld	l, -8868(ix)
	ld	h, -8867(ix)
	push	hl
	ld	l, -8798(ix)
	ld	h, -8797(ix)
	push	hl
	ld	l, -8760(ix)
	ld	h, -8759(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1809
	dec	sp
	dec	sp
	ld	-8870(ix), l
	ld	-8869(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8872(ix), l
	ld	-8871(ix), h
	ld	l, -8872(ix)
	ld	h, -8871(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8880(ix), l
	ld	-8879(ix), h
	ld	l, -8880(ix)
	ld	h, -8879(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59639
	ld	hl, #0
	jp	__cmp_e_49833
__cmp_t_59639:
	ld	hl, #1
__cmp_e_49833:
	dec	sp
	dec	sp
	ld	-8882(ix), l
	ld	-8881(ix), h
	ld	l, -8882(ix)
	ld	h, -8881(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_848
	ld	hl, #0
	jp	__cmp_e_96358
__cmp_t_848:
	ld	hl, #1
__cmp_e_96358:
	dec	sp
	dec	sp
	ld	-8884(ix), l
	ld	-8883(ix), h
	ld	l, -8884(ix)
	ld	h, -8883(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1813
	jp	__xcc_L1814
__xcc_L1814:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8886(ix), l
	ld	-8885(ix), h
	ld	l, -8886(ix)
	ld	h, -8885(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8894(ix), l
	ld	-8893(ix), h
	ld	l, -8894(ix)
	ld	h, -8893(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8896(ix), l
	ld	-8895(ix), h
	ld	l, -8896(ix)
	ld	h, -8895(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_43444
	ld	hl, #0
	jp	__cmp_e_98623
__cmp_t_43444:
	ld	hl, #1
__cmp_e_98623:
	dec	sp
	dec	sp
	ld	-8898(ix), l
	ld	-8897(ix), h
	ld	l, -8898(ix)
	ld	h, -8897(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_15055
	ld	hl, #0
	jp	__cmp_e_11310
__cmp_t_15055:
	ld	hl, #1
__cmp_e_11310:
	dec	sp
	dec	sp
	ld	-8900(ix), l
	ld	-8899(ix), h
	jp	__xcc_L1815
__xcc_L1813:
	ld	hl, #1
	ld	-8900(ix), l
	ld	-8899(ix), h
__xcc_L1815:
	ld	l, -8900(ix)
	ld	h, -8899(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1810
	jp	__xcc_L1811
__xcc_L1810:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8902(ix), l
	ld	-8901(ix), h
	ld	l, -8902(ix)
	ld	h, -8901(ix)
	dec	sp
	dec	sp
	ld	-8904(ix), l
	ld	-8903(ix), h
	jp	__xcc_L1812
__xcc_L1811:
	ld	hl, #1
	ld	-8904(ix), l
	ld	-8903(ix), h
__xcc_L1812:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8906(ix), l
	ld	-8905(ix), h
	.globl __mul16
	ld	l, -8906(ix)
	ld	h, -8905(ix)
	push	hl
	ld	l, -8904(ix)
	ld	h, -8903(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8908(ix), l
	ld	-8907(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8910(ix), l
	ld	-8909(ix), h
	ld	l, -8910(ix)
	ld	h, -8909(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8918(ix), l
	ld	-8917(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8926(ix), l
	ld	-8925(ix), h
	ld	l, -8918(ix)
	ld	h, -8917(ix)
	push	hl
	ld	l, -8926(ix)
	ld	h, -8925(ix)
	ld	b, l
	pop	hl
__shift_3485:
	ld	a, b
	or	a, a
	jp	z, __sdone_4589
	add	hl, hl
	djnz	__shift_3485
__sdone_4589:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8934(ix), l
	ld	-8933(ix), h
	ld	l, -8934(ix)
	ld	h, -8933(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_37283
	ld	hl, #0
	jp	__cmp_e_15644
__cmp_t_37283:
	ld	hl, #1
__cmp_e_15644:
	dec	sp
	dec	sp
	ld	-8936(ix), l
	ld	-8935(ix), h
	ld	l, -8936(ix)
	ld	h, -8935(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_61246
	ld	hl, #0
	jp	__cmp_e_35412
__cmp_t_61246:
	ld	hl, #1
__cmp_e_35412:
	dec	sp
	dec	sp
	ld	-8938(ix), l
	ld	-8937(ix), h
	ld	l, -8938(ix)
	ld	h, -8937(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1819
	jp	__xcc_L1820
__xcc_L1820:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8940(ix), l
	ld	-8939(ix), h
	ld	l, -8940(ix)
	ld	h, -8939(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8948(ix), l
	ld	-8947(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8956(ix), l
	ld	-8955(ix), h
	ld	l, -8948(ix)
	ld	h, -8947(ix)
	push	hl
	ld	l, -8956(ix)
	ld	h, -8955(ix)
	ld	b, l
	pop	hl
__shift_6555:
	ld	a, b
	or	a, a
	jp	z, __sdone_4893
	add	hl, hl
	djnz	__shift_6555
__sdone_4893:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8964(ix), l
	ld	-8963(ix), h
	ld	l, -8964(ix)
	ld	h, -8963(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8966(ix), l
	ld	-8965(ix), h
	ld	l, -8966(ix)
	ld	h, -8965(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_63042
	ld	hl, #0
	jp	__cmp_e_92300
__cmp_t_63042:
	ld	hl, #1
__cmp_e_92300:
	dec	sp
	dec	sp
	ld	-8968(ix), l
	ld	-8967(ix), h
	ld	l, -8968(ix)
	ld	h, -8967(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_99950
	ld	hl, #0
	jp	__cmp_e_61120
__cmp_t_99950:
	ld	hl, #1
__cmp_e_61120:
	dec	sp
	dec	sp
	ld	-8970(ix), l
	ld	-8969(ix), h
	jp	__xcc_L1821
__xcc_L1819:
	ld	hl, #1
	ld	-8970(ix), l
	ld	-8969(ix), h
__xcc_L1821:
	ld	l, -8970(ix)
	ld	h, -8969(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1816
	jp	__xcc_L1817
__xcc_L1816:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8972(ix), l
	ld	-8971(ix), h
	ld	l, -8972(ix)
	ld	h, -8971(ix)
	dec	sp
	dec	sp
	ld	-8974(ix), l
	ld	-8973(ix), h
	jp	__xcc_L1818
__xcc_L1817:
	ld	hl, #1
	ld	-8974(ix), l
	ld	-8973(ix), h
__xcc_L1818:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-8976(ix), l
	ld	-8975(ix), h
	.globl __mul16
	ld	l, -8976(ix)
	ld	h, -8975(ix)
	push	hl
	ld	l, -8974(ix)
	ld	h, -8973(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8978(ix), l
	ld	-8977(ix), h
	ld	l, -8978(ix)
	ld	h, -8977(ix)
	push	hl
	ld	l, -8908(ix)
	ld	h, -8907(ix)
	push	hl
	ld	l, -8870(ix)
	ld	h, -8869(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1794:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1793
	jp	__xcc_L1795
__xcc_L1795:
__xcc_L1704:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1703
	jp	__xcc_L1705
__xcc_L1705:
__xcc_L1822:
__xcc_L1825:
	ld	hl, #__str_1828
	dec	sp
	dec	sp
	ld	-8980(ix), l
	ld	-8979(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8982(ix), l
	ld	-8981(ix), h
	ld	l, -8982(ix)
	ld	h, -8981(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8990(ix), l
	ld	-8989(ix), h
	ld	l, -8990(ix)
	ld	h, -8989(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_75789
	ld	hl, #0
	jp	__cmp_e_53428
__cmp_t_75789:
	ld	hl, #1
__cmp_e_53428:
	dec	sp
	dec	sp
	ld	-8992(ix), l
	ld	-8991(ix), h
	ld	l, -8992(ix)
	ld	h, -8991(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_46550
	ld	hl, #0
	jp	__cmp_e_41549
__cmp_t_46550:
	ld	hl, #1
__cmp_e_41549:
	dec	sp
	dec	sp
	ld	-8994(ix), l
	ld	-8993(ix), h
	ld	l, -8994(ix)
	ld	h, -8993(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1832
	jp	__xcc_L1833
__xcc_L1833:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-8996(ix), l
	ld	-8995(ix), h
	ld	l, -8996(ix)
	ld	h, -8995(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9004(ix), l
	ld	-9003(ix), h
	ld	l, -9004(ix)
	ld	h, -9003(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9006(ix), l
	ld	-9005(ix), h
	ld	l, -9006(ix)
	ld	h, -9005(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_60056
	ld	hl, #0
	jp	__cmp_e_20737
__cmp_t_60056:
	ld	hl, #1
__cmp_e_20737:
	dec	sp
	dec	sp
	ld	-9008(ix), l
	ld	-9007(ix), h
	ld	l, -9008(ix)
	ld	h, -9007(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47042
	ld	hl, #0
	jp	__cmp_e_78241
__cmp_t_47042:
	ld	hl, #1
__cmp_e_78241:
	dec	sp
	dec	sp
	ld	-9010(ix), l
	ld	-9009(ix), h
	jp	__xcc_L1834
__xcc_L1832:
	ld	hl, #1
	ld	-9010(ix), l
	ld	-9009(ix), h
__xcc_L1834:
	ld	l, -9010(ix)
	ld	h, -9009(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1829
	jp	__xcc_L1830
__xcc_L1829:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9012(ix), l
	ld	-9011(ix), h
	ld	l, -9012(ix)
	ld	h, -9011(ix)
	dec	sp
	dec	sp
	ld	-9014(ix), l
	ld	-9013(ix), h
	jp	__xcc_L1831
__xcc_L1830:
	ld	hl, #1
	ld	-9014(ix), l
	ld	-9013(ix), h
__xcc_L1831:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9016(ix), l
	ld	-9015(ix), h
	.globl __mul16
	ld	l, -9016(ix)
	ld	h, -9015(ix)
	push	hl
	ld	l, -9014(ix)
	ld	h, -9013(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9018(ix), l
	ld	-9017(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9020(ix), l
	ld	-9019(ix), h
	ld	l, -9020(ix)
	ld	h, -9019(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9028(ix), l
	ld	-9027(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9030(ix), l
	ld	-9029(ix), h
	ld	l, -9028(ix)
	ld	h, -9027(ix)
	push	hl
	ld	l, -9030(ix)
	ld	h, -9029(ix)
	ld	b, l
	pop	hl
__shift_5914:
	ld	a, b
	or	a, a
	jp	z, __sdone_1924
	add	hl, hl
	djnz	__shift_5914
__sdone_1924:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9038(ix), l
	ld	-9037(ix), h
	ld	l, -9038(ix)
	ld	h, -9037(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_83458
	ld	hl, #0
	jp	__cmp_e_25554
__cmp_t_83458:
	ld	hl, #1
__cmp_e_25554:
	dec	sp
	dec	sp
	ld	-9040(ix), l
	ld	-9039(ix), h
	ld	l, -9040(ix)
	ld	h, -9039(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_88109
	ld	hl, #0
	jp	__cmp_e_658
__cmp_t_88109:
	ld	hl, #1
__cmp_e_658:
	dec	sp
	dec	sp
	ld	-9042(ix), l
	ld	-9041(ix), h
	ld	l, -9042(ix)
	ld	h, -9041(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1838
	jp	__xcc_L1839
__xcc_L1839:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9044(ix), l
	ld	-9043(ix), h
	ld	l, -9044(ix)
	ld	h, -9043(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9052(ix), l
	ld	-9051(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9054(ix), l
	ld	-9053(ix), h
	ld	l, -9052(ix)
	ld	h, -9051(ix)
	push	hl
	ld	l, -9054(ix)
	ld	h, -9053(ix)
	ld	b, l
	pop	hl
__shift_8264:
	ld	a, b
	or	a, a
	jp	z, __sdone_1554
	add	hl, hl
	djnz	__shift_8264
__sdone_1554:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9062(ix), l
	ld	-9061(ix), h
	ld	l, -9062(ix)
	ld	h, -9061(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9064(ix), l
	ld	-9063(ix), h
	ld	l, -9064(ix)
	ld	h, -9063(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_99281
	ld	hl, #0
	jp	__cmp_e_53319
__cmp_t_99281:
	ld	hl, #1
__cmp_e_53319:
	dec	sp
	dec	sp
	ld	-9066(ix), l
	ld	-9065(ix), h
	ld	l, -9066(ix)
	ld	h, -9065(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_59216
	ld	hl, #0
	jp	__cmp_e_9118
__cmp_t_59216:
	ld	hl, #1
__cmp_e_9118:
	dec	sp
	dec	sp
	ld	-9068(ix), l
	ld	-9067(ix), h
	jp	__xcc_L1840
__xcc_L1838:
	ld	hl, #1
	ld	-9068(ix), l
	ld	-9067(ix), h
__xcc_L1840:
	ld	l, -9068(ix)
	ld	h, -9067(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1835
	jp	__xcc_L1836
__xcc_L1835:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9070(ix), l
	ld	-9069(ix), h
	ld	l, -9070(ix)
	ld	h, -9069(ix)
	dec	sp
	dec	sp
	ld	-9072(ix), l
	ld	-9071(ix), h
	jp	__xcc_L1837
__xcc_L1836:
	ld	hl, #1
	ld	-9072(ix), l
	ld	-9071(ix), h
__xcc_L1837:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9074(ix), l
	ld	-9073(ix), h
	.globl __mul16
	ld	l, -9074(ix)
	ld	h, -9073(ix)
	push	hl
	ld	l, -9072(ix)
	ld	h, -9071(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9076(ix), l
	ld	-9075(ix), h
	ld	l, -9076(ix)
	ld	h, -9075(ix)
	push	hl
	ld	l, -9018(ix)
	ld	h, -9017(ix)
	push	hl
	ld	l, -8980(ix)
	ld	h, -8979(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1841
	dec	sp
	dec	sp
	ld	-9078(ix), l
	ld	-9077(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9080(ix), l
	ld	-9079(ix), h
	ld	l, -9080(ix)
	ld	h, -9079(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9088(ix), l
	ld	-9087(ix), h
	ld	l, -9088(ix)
	ld	h, -9087(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_24260
	ld	hl, #0
	jp	__cmp_e_96499
__cmp_t_24260:
	ld	hl, #1
__cmp_e_96499:
	dec	sp
	dec	sp
	ld	-9090(ix), l
	ld	-9089(ix), h
	ld	l, -9090(ix)
	ld	h, -9089(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_24763
	ld	hl, #0
	jp	__cmp_e_85506
__cmp_t_24763:
	ld	hl, #1
__cmp_e_85506:
	dec	sp
	dec	sp
	ld	-9092(ix), l
	ld	-9091(ix), h
	ld	l, -9092(ix)
	ld	h, -9091(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1845
	jp	__xcc_L1846
__xcc_L1846:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9094(ix), l
	ld	-9093(ix), h
	ld	l, -9094(ix)
	ld	h, -9093(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9102(ix), l
	ld	-9101(ix), h
	ld	l, -9102(ix)
	ld	h, -9101(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9104(ix), l
	ld	-9103(ix), h
	ld	l, -9104(ix)
	ld	h, -9103(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_48263
	ld	hl, #0
	jp	__cmp_e_51318
__cmp_t_48263:
	ld	hl, #1
__cmp_e_51318:
	dec	sp
	dec	sp
	ld	-9106(ix), l
	ld	-9105(ix), h
	ld	l, -9106(ix)
	ld	h, -9105(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_10399
	ld	hl, #0
	jp	__cmp_e_11305
__cmp_t_10399:
	ld	hl, #1
__cmp_e_11305:
	dec	sp
	dec	sp
	ld	-9108(ix), l
	ld	-9107(ix), h
	jp	__xcc_L1847
__xcc_L1845:
	ld	hl, #1
	ld	-9108(ix), l
	ld	-9107(ix), h
__xcc_L1847:
	ld	l, -9108(ix)
	ld	h, -9107(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1842
	jp	__xcc_L1843
__xcc_L1842:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9110(ix), l
	ld	-9109(ix), h
	ld	l, -9110(ix)
	ld	h, -9109(ix)
	dec	sp
	dec	sp
	ld	-9112(ix), l
	ld	-9111(ix), h
	jp	__xcc_L1844
__xcc_L1843:
	ld	hl, #1
	ld	-9112(ix), l
	ld	-9111(ix), h
__xcc_L1844:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9114(ix), l
	ld	-9113(ix), h
	.globl __mul16
	ld	l, -9114(ix)
	ld	h, -9113(ix)
	push	hl
	ld	l, -9112(ix)
	ld	h, -9111(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9116(ix), l
	ld	-9115(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9118(ix), l
	ld	-9117(ix), h
	ld	l, -9118(ix)
	ld	h, -9117(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9126(ix), l
	ld	-9125(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9128(ix), l
	ld	-9127(ix), h
	ld	l, -9126(ix)
	ld	h, -9125(ix)
	push	hl
	ld	l, -9128(ix)
	ld	h, -9127(ix)
	ld	b, l
	pop	hl
__shift_9970:
	ld	a, b
	or	a, a
	jp	z, __sdone_6701
	add	hl, hl
	djnz	__shift_9970
__sdone_6701:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9136(ix), l
	ld	-9135(ix), h
	ld	l, -9136(ix)
	ld	h, -9135(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_88777
	ld	hl, #0
	jp	__cmp_e_52111
__cmp_t_88777:
	ld	hl, #1
__cmp_e_52111:
	dec	sp
	dec	sp
	ld	-9138(ix), l
	ld	-9137(ix), h
	ld	l, -9138(ix)
	ld	h, -9137(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80129
	ld	hl, #0
	jp	__cmp_e_35328
__cmp_t_80129:
	ld	hl, #1
__cmp_e_35328:
	dec	sp
	dec	sp
	ld	-9140(ix), l
	ld	-9139(ix), h
	ld	l, -9140(ix)
	ld	h, -9139(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1851
	jp	__xcc_L1852
__xcc_L1852:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9142(ix), l
	ld	-9141(ix), h
	ld	l, -9142(ix)
	ld	h, -9141(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9150(ix), l
	ld	-9149(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9152(ix), l
	ld	-9151(ix), h
	ld	l, -9150(ix)
	ld	h, -9149(ix)
	push	hl
	ld	l, -9152(ix)
	ld	h, -9151(ix)
	ld	b, l
	pop	hl
__shift_12:
	ld	a, b
	or	a, a
	jp	z, __sdone_186
	add	hl, hl
	djnz	__shift_12
__sdone_186:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9160(ix), l
	ld	-9159(ix), h
	ld	l, -9160(ix)
	ld	h, -9159(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9162(ix), l
	ld	-9161(ix), h
	ld	l, -9162(ix)
	ld	h, -9161(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_72417
	ld	hl, #0
	jp	__cmp_e_57055
__cmp_t_72417:
	ld	hl, #1
__cmp_e_57055:
	dec	sp
	dec	sp
	ld	-9164(ix), l
	ld	-9163(ix), h
	ld	l, -9164(ix)
	ld	h, -9163(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_34779
	ld	hl, #0
	jp	__cmp_e_54683
__cmp_t_34779:
	ld	hl, #1
__cmp_e_54683:
	dec	sp
	dec	sp
	ld	-9166(ix), l
	ld	-9165(ix), h
	jp	__xcc_L1853
__xcc_L1851:
	ld	hl, #1
	ld	-9166(ix), l
	ld	-9165(ix), h
__xcc_L1853:
	ld	l, -9166(ix)
	ld	h, -9165(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1848
	jp	__xcc_L1849
__xcc_L1848:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9168(ix), l
	ld	-9167(ix), h
	ld	l, -9168(ix)
	ld	h, -9167(ix)
	dec	sp
	dec	sp
	ld	-9170(ix), l
	ld	-9169(ix), h
	jp	__xcc_L1850
__xcc_L1849:
	ld	hl, #1
	ld	-9170(ix), l
	ld	-9169(ix), h
__xcc_L1850:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9172(ix), l
	ld	-9171(ix), h
	.globl __mul16
	ld	l, -9172(ix)
	ld	h, -9171(ix)
	push	hl
	ld	l, -9170(ix)
	ld	h, -9169(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9174(ix), l
	ld	-9173(ix), h
	ld	l, -9174(ix)
	ld	h, -9173(ix)
	push	hl
	ld	l, -9116(ix)
	ld	h, -9115(ix)
	push	hl
	ld	l, -9078(ix)
	ld	h, -9077(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1826:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1825
	jp	__xcc_L1827
__xcc_L1827:
__xcc_L1854:
	ld	hl, #__str_1857
	dec	sp
	dec	sp
	ld	-9176(ix), l
	ld	-9175(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9178(ix), l
	ld	-9177(ix), h
	ld	l, -9178(ix)
	ld	h, -9177(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9186(ix), l
	ld	-9185(ix), h
	ld	l, -9186(ix)
	ld	h, -9185(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78979
	ld	hl, #0
	jp	__cmp_e_34589
__cmp_t_78979:
	ld	hl, #1
__cmp_e_34589:
	dec	sp
	dec	sp
	ld	-9188(ix), l
	ld	-9187(ix), h
	ld	l, -9188(ix)
	ld	h, -9187(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80237
	ld	hl, #0
	jp	__cmp_e_67089
__cmp_t_80237:
	ld	hl, #1
__cmp_e_67089:
	dec	sp
	dec	sp
	ld	-9190(ix), l
	ld	-9189(ix), h
	ld	l, -9190(ix)
	ld	h, -9189(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1861
	jp	__xcc_L1862
__xcc_L1862:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9192(ix), l
	ld	-9191(ix), h
	ld	l, -9192(ix)
	ld	h, -9191(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9200(ix), l
	ld	-9199(ix), h
	ld	l, -9200(ix)
	ld	h, -9199(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9202(ix), l
	ld	-9201(ix), h
	ld	l, -9202(ix)
	ld	h, -9201(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_35247
	ld	hl, #0
	jp	__cmp_e_34853
__cmp_t_35247:
	ld	hl, #1
__cmp_e_34853:
	dec	sp
	dec	sp
	ld	-9204(ix), l
	ld	-9203(ix), h
	ld	l, -9204(ix)
	ld	h, -9203(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_14995
	ld	hl, #0
	jp	__cmp_e_50880
__cmp_t_14995:
	ld	hl, #1
__cmp_e_50880:
	dec	sp
	dec	sp
	ld	-9206(ix), l
	ld	-9205(ix), h
	jp	__xcc_L1863
__xcc_L1861:
	ld	hl, #1
	ld	-9206(ix), l
	ld	-9205(ix), h
__xcc_L1863:
	ld	l, -9206(ix)
	ld	h, -9205(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1858
	jp	__xcc_L1859
__xcc_L1858:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9208(ix), l
	ld	-9207(ix), h
	ld	l, -9208(ix)
	ld	h, -9207(ix)
	dec	sp
	dec	sp
	ld	-9210(ix), l
	ld	-9209(ix), h
	jp	__xcc_L1860
__xcc_L1859:
	ld	hl, #1
	ld	-9210(ix), l
	ld	-9209(ix), h
__xcc_L1860:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9212(ix), l
	ld	-9211(ix), h
	.globl __mul16
	ld	l, -9212(ix)
	ld	h, -9211(ix)
	push	hl
	ld	l, -9210(ix)
	ld	h, -9209(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9214(ix), l
	ld	-9213(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9216(ix), l
	ld	-9215(ix), h
	ld	l, -9216(ix)
	ld	h, -9215(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9224(ix), l
	ld	-9223(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9226(ix), l
	ld	-9225(ix), h
	ld	l, -9224(ix)
	ld	h, -9223(ix)
	push	hl
	ld	l, -9226(ix)
	ld	h, -9225(ix)
	ld	b, l
	pop	hl
__shift_4524:
	ld	a, b
	or	a, a
	jp	z, __sdone_4211
	add	hl, hl
	djnz	__shift_4524
__sdone_4211:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9234(ix), l
	ld	-9233(ix), h
	ld	l, -9234(ix)
	ld	h, -9233(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_59999
	ld	hl, #0
	jp	__cmp_e_28784
__cmp_t_59999:
	ld	hl, #1
__cmp_e_28784:
	dec	sp
	dec	sp
	ld	-9236(ix), l
	ld	-9235(ix), h
	ld	l, -9236(ix)
	ld	h, -9235(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_70711
	ld	hl, #0
	jp	__cmp_e_1114
__cmp_t_70711:
	ld	hl, #1
__cmp_e_1114:
	dec	sp
	dec	sp
	ld	-9238(ix), l
	ld	-9237(ix), h
	ld	l, -9238(ix)
	ld	h, -9237(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1867
	jp	__xcc_L1868
__xcc_L1868:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9240(ix), l
	ld	-9239(ix), h
	ld	l, -9240(ix)
	ld	h, -9239(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9248(ix), l
	ld	-9247(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9250(ix), l
	ld	-9249(ix), h
	ld	l, -9248(ix)
	ld	h, -9247(ix)
	push	hl
	ld	l, -9250(ix)
	ld	h, -9249(ix)
	ld	b, l
	pop	hl
__shift_643:
	ld	a, b
	or	a, a
	jp	z, __sdone_5326
	add	hl, hl
	djnz	__shift_643
__sdone_5326:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9258(ix), l
	ld	-9257(ix), h
	ld	l, -9258(ix)
	ld	h, -9257(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9260(ix), l
	ld	-9259(ix), h
	ld	l, -9260(ix)
	ld	h, -9259(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_68784
	ld	hl, #0
	jp	__cmp_e_41042
__cmp_t_68784:
	ld	hl, #1
__cmp_e_41042:
	dec	sp
	dec	sp
	ld	-9262(ix), l
	ld	-9261(ix), h
	ld	l, -9262(ix)
	ld	h, -9261(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_46632
	ld	hl, #0
	jp	__cmp_e_45107
__cmp_t_46632:
	ld	hl, #1
__cmp_e_45107:
	dec	sp
	dec	sp
	ld	-9264(ix), l
	ld	-9263(ix), h
	jp	__xcc_L1869
__xcc_L1867:
	ld	hl, #1
	ld	-9264(ix), l
	ld	-9263(ix), h
__xcc_L1869:
	ld	l, -9264(ix)
	ld	h, -9263(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1864
	jp	__xcc_L1865
__xcc_L1864:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9266(ix), l
	ld	-9265(ix), h
	ld	l, -9266(ix)
	ld	h, -9265(ix)
	dec	sp
	dec	sp
	ld	-9268(ix), l
	ld	-9267(ix), h
	jp	__xcc_L1866
__xcc_L1865:
	ld	hl, #1
	ld	-9268(ix), l
	ld	-9267(ix), h
__xcc_L1866:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9270(ix), l
	ld	-9269(ix), h
	.globl __mul16
	ld	l, -9270(ix)
	ld	h, -9269(ix)
	push	hl
	ld	l, -9268(ix)
	ld	h, -9267(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9272(ix), l
	ld	-9271(ix), h
	ld	l, -9272(ix)
	ld	h, -9271(ix)
	push	hl
	ld	l, -9214(ix)
	ld	h, -9213(ix)
	push	hl
	ld	l, -9176(ix)
	ld	h, -9175(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1870
	dec	sp
	dec	sp
	ld	-9274(ix), l
	ld	-9273(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9276(ix), l
	ld	-9275(ix), h
	ld	l, -9276(ix)
	ld	h, -9275(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9284(ix), l
	ld	-9283(ix), h
	ld	l, -9284(ix)
	ld	h, -9283(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84096
	ld	hl, #0
	jp	__cmp_e_51761
__cmp_t_84096:
	ld	hl, #1
__cmp_e_51761:
	dec	sp
	dec	sp
	ld	-9286(ix), l
	ld	-9285(ix), h
	ld	l, -9286(ix)
	ld	h, -9285(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97218
	ld	hl, #0
	jp	__cmp_e_64225
__cmp_t_97218:
	ld	hl, #1
__cmp_e_64225:
	dec	sp
	dec	sp
	ld	-9288(ix), l
	ld	-9287(ix), h
	ld	l, -9288(ix)
	ld	h, -9287(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1874
	jp	__xcc_L1875
__xcc_L1875:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9290(ix), l
	ld	-9289(ix), h
	ld	l, -9290(ix)
	ld	h, -9289(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9298(ix), l
	ld	-9297(ix), h
	ld	l, -9298(ix)
	ld	h, -9297(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9300(ix), l
	ld	-9299(ix), h
	ld	l, -9300(ix)
	ld	h, -9299(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_3441
	ld	hl, #0
	jp	__cmp_e_7231
__cmp_t_3441:
	ld	hl, #1
__cmp_e_7231:
	dec	sp
	dec	sp
	ld	-9302(ix), l
	ld	-9301(ix), h
	ld	l, -9302(ix)
	ld	h, -9301(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_20763
	ld	hl, #0
	jp	__cmp_e_92210
__cmp_t_20763:
	ld	hl, #1
__cmp_e_92210:
	dec	sp
	dec	sp
	ld	-9304(ix), l
	ld	-9303(ix), h
	jp	__xcc_L1876
__xcc_L1874:
	ld	hl, #1
	ld	-9304(ix), l
	ld	-9303(ix), h
__xcc_L1876:
	ld	l, -9304(ix)
	ld	h, -9303(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1871
	jp	__xcc_L1872
__xcc_L1871:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9306(ix), l
	ld	-9305(ix), h
	ld	l, -9306(ix)
	ld	h, -9305(ix)
	dec	sp
	dec	sp
	ld	-9308(ix), l
	ld	-9307(ix), h
	jp	__xcc_L1873
__xcc_L1872:
	ld	hl, #1
	ld	-9308(ix), l
	ld	-9307(ix), h
__xcc_L1873:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9310(ix), l
	ld	-9309(ix), h
	.globl __mul16
	ld	l, -9310(ix)
	ld	h, -9309(ix)
	push	hl
	ld	l, -9308(ix)
	ld	h, -9307(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9312(ix), l
	ld	-9311(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9314(ix), l
	ld	-9313(ix), h
	ld	l, -9314(ix)
	ld	h, -9313(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9322(ix), l
	ld	-9321(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9324(ix), l
	ld	-9323(ix), h
	ld	l, -9322(ix)
	ld	h, -9321(ix)
	push	hl
	ld	l, -9324(ix)
	ld	h, -9323(ix)
	ld	b, l
	pop	hl
__shift_4286:
	ld	a, b
	or	a, a
	jp	z, __sdone_1894
	add	hl, hl
	djnz	__shift_4286
__sdone_1894:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9332(ix), l
	ld	-9331(ix), h
	ld	l, -9332(ix)
	ld	h, -9331(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_46894
	ld	hl, #0
	jp	__cmp_e_59617
__cmp_t_46894:
	ld	hl, #1
__cmp_e_59617:
	dec	sp
	dec	sp
	ld	-9334(ix), l
	ld	-9333(ix), h
	ld	l, -9334(ix)
	ld	h, -9333(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_22835
	ld	hl, #0
	jp	__cmp_e_43483
__cmp_t_22835:
	ld	hl, #1
__cmp_e_43483:
	dec	sp
	dec	sp
	ld	-9336(ix), l
	ld	-9335(ix), h
	ld	l, -9336(ix)
	ld	h, -9335(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1880
	jp	__xcc_L1881
__xcc_L1881:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9338(ix), l
	ld	-9337(ix), h
	ld	l, -9338(ix)
	ld	h, -9337(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9346(ix), l
	ld	-9345(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	ld	-9348(ix), l
	ld	-9347(ix), h
	ld	l, -9346(ix)
	ld	h, -9345(ix)
	push	hl
	ld	l, -9348(ix)
	ld	h, -9347(ix)
	ld	b, l
	pop	hl
__shift_3058:
	ld	a, b
	or	a, a
	jp	z, __sdone_8082
	add	hl, hl
	djnz	__shift_3058
__sdone_8082:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9356(ix), l
	ld	-9355(ix), h
	ld	l, -9356(ix)
	ld	h, -9355(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9358(ix), l
	ld	-9357(ix), h
	ld	l, -9358(ix)
	ld	h, -9357(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_78337
	ld	hl, #0
	jp	__cmp_e_58053
__cmp_t_78337:
	ld	hl, #1
__cmp_e_58053:
	dec	sp
	dec	sp
	ld	-9360(ix), l
	ld	-9359(ix), h
	ld	l, -9360(ix)
	ld	h, -9359(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_25315
	ld	hl, #0
	jp	__cmp_e_99213
__cmp_t_25315:
	ld	hl, #1
__cmp_e_99213:
	dec	sp
	dec	sp
	ld	-9362(ix), l
	ld	-9361(ix), h
	jp	__xcc_L1882
__xcc_L1880:
	ld	hl, #1
	ld	-9362(ix), l
	ld	-9361(ix), h
__xcc_L1882:
	ld	l, -9362(ix)
	ld	h, -9361(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1877
	jp	__xcc_L1878
__xcc_L1877:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9364(ix), l
	ld	-9363(ix), h
	ld	l, -9364(ix)
	ld	h, -9363(ix)
	dec	sp
	dec	sp
	ld	-9366(ix), l
	ld	-9365(ix), h
	jp	__xcc_L1879
__xcc_L1878:
	ld	hl, #1
	ld	-9366(ix), l
	ld	-9365(ix), h
__xcc_L1879:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9368(ix), l
	ld	-9367(ix), h
	.globl __mul16
	ld	l, -9368(ix)
	ld	h, -9367(ix)
	push	hl
	ld	l, -9366(ix)
	ld	h, -9365(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9370(ix), l
	ld	-9369(ix), h
	ld	l, -9370(ix)
	ld	h, -9369(ix)
	push	hl
	ld	l, -9312(ix)
	ld	h, -9311(ix)
	push	hl
	ld	l, -9274(ix)
	ld	h, -9273(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1855:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1854
	jp	__xcc_L1856
__xcc_L1856:
__xcc_L1883:
	ld	hl, #__str_1886
	dec	sp
	dec	sp
	ld	-9372(ix), l
	ld	-9371(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9374(ix), l
	ld	-9373(ix), h
	ld	l, -9374(ix)
	ld	h, -9373(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9382(ix), l
	ld	-9381(ix), h
	ld	l, -9382(ix)
	ld	h, -9381(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_48617
	ld	hl, #0
	jp	__cmp_e_85314
__cmp_t_48617:
	ld	hl, #1
__cmp_e_85314:
	dec	sp
	dec	sp
	ld	-9384(ix), l
	ld	-9383(ix), h
	ld	l, -9384(ix)
	ld	h, -9383(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_27998
	ld	hl, #0
	jp	__cmp_e_35680
__cmp_t_27998:
	ld	hl, #1
__cmp_e_35680:
	dec	sp
	dec	sp
	ld	-9386(ix), l
	ld	-9385(ix), h
	ld	l, -9386(ix)
	ld	h, -9385(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1890
	jp	__xcc_L1891
__xcc_L1891:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9388(ix), l
	ld	-9387(ix), h
	ld	l, -9388(ix)
	ld	h, -9387(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9396(ix), l
	ld	-9395(ix), h
	ld	l, -9396(ix)
	ld	h, -9395(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9398(ix), l
	ld	-9397(ix), h
	ld	l, -9398(ix)
	ld	h, -9397(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_2780
	ld	hl, #0
	jp	__cmp_e_58641
__cmp_t_2780:
	ld	hl, #1
__cmp_e_58641:
	dec	sp
	dec	sp
	ld	-9400(ix), l
	ld	-9399(ix), h
	ld	l, -9400(ix)
	ld	h, -9399(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_71006
	ld	hl, #0
	jp	__cmp_e_71564
__cmp_t_71006:
	ld	hl, #1
__cmp_e_71564:
	dec	sp
	dec	sp
	ld	-9402(ix), l
	ld	-9401(ix), h
	jp	__xcc_L1892
__xcc_L1890:
	ld	hl, #1
	ld	-9402(ix), l
	ld	-9401(ix), h
__xcc_L1892:
	ld	l, -9402(ix)
	ld	h, -9401(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1887
	jp	__xcc_L1888
__xcc_L1887:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9404(ix), l
	ld	-9403(ix), h
	ld	l, -9404(ix)
	ld	h, -9403(ix)
	dec	sp
	dec	sp
	ld	-9406(ix), l
	ld	-9405(ix), h
	jp	__xcc_L1889
__xcc_L1888:
	ld	hl, #1
	ld	-9406(ix), l
	ld	-9405(ix), h
__xcc_L1889:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9408(ix), l
	ld	-9407(ix), h
	.globl __mul16
	ld	l, -9408(ix)
	ld	h, -9407(ix)
	push	hl
	ld	l, -9406(ix)
	ld	h, -9405(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9410(ix), l
	ld	-9409(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9412(ix), l
	ld	-9411(ix), h
	ld	l, -9412(ix)
	ld	h, -9411(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9420(ix), l
	ld	-9419(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9424(ix), l
	ld	-9423(ix), h
	ld	l, -9420(ix)
	ld	h, -9419(ix)
	push	hl
	ld	l, -9424(ix)
	ld	h, -9423(ix)
	ld	b, l
	pop	hl
__shift_6035:
	ld	a, b
	or	a, a
	jp	z, __sdone_7638
	add	hl, hl
	djnz	__shift_6035
__sdone_7638:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9432(ix), l
	ld	-9431(ix), h
	ld	l, -9432(ix)
	ld	h, -9431(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_33023
	ld	hl, #0
	jp	__cmp_e_131
__cmp_t_33023:
	ld	hl, #1
__cmp_e_131:
	dec	sp
	dec	sp
	ld	-9434(ix), l
	ld	-9433(ix), h
	ld	l, -9434(ix)
	ld	h, -9433(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85752
	ld	hl, #0
	jp	__cmp_e_30242
__cmp_t_85752:
	ld	hl, #1
__cmp_e_30242:
	dec	sp
	dec	sp
	ld	-9436(ix), l
	ld	-9435(ix), h
	ld	l, -9436(ix)
	ld	h, -9435(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1896
	jp	__xcc_L1897
__xcc_L1897:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9438(ix), l
	ld	-9437(ix), h
	ld	l, -9438(ix)
	ld	h, -9437(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9446(ix), l
	ld	-9445(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9450(ix), l
	ld	-9449(ix), h
	ld	l, -9446(ix)
	ld	h, -9445(ix)
	push	hl
	ld	l, -9450(ix)
	ld	h, -9449(ix)
	ld	b, l
	pop	hl
__shift_709:
	ld	a, b
	or	a, a
	jp	z, __sdone_5545
	add	hl, hl
	djnz	__shift_709
__sdone_5545:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9458(ix), l
	ld	-9457(ix), h
	ld	l, -9458(ix)
	ld	h, -9457(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9460(ix), l
	ld	-9459(ix), h
	ld	l, -9460(ix)
	ld	h, -9459(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_53825
	ld	hl, #0
	jp	__cmp_e_17824
__cmp_t_53825:
	ld	hl, #1
__cmp_e_17824:
	dec	sp
	dec	sp
	ld	-9462(ix), l
	ld	-9461(ix), h
	ld	l, -9462(ix)
	ld	h, -9461(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97756
	ld	hl, #0
	jp	__cmp_e_34463
__cmp_t_97756:
	ld	hl, #1
__cmp_e_34463:
	dec	sp
	dec	sp
	ld	-9464(ix), l
	ld	-9463(ix), h
	jp	__xcc_L1898
__xcc_L1896:
	ld	hl, #1
	ld	-9464(ix), l
	ld	-9463(ix), h
__xcc_L1898:
	ld	l, -9464(ix)
	ld	h, -9463(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1893
	jp	__xcc_L1894
__xcc_L1893:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9466(ix), l
	ld	-9465(ix), h
	ld	l, -9466(ix)
	ld	h, -9465(ix)
	dec	sp
	dec	sp
	ld	-9468(ix), l
	ld	-9467(ix), h
	jp	__xcc_L1895
__xcc_L1894:
	ld	hl, #1
	ld	-9468(ix), l
	ld	-9467(ix), h
__xcc_L1895:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9470(ix), l
	ld	-9469(ix), h
	.globl __mul16
	ld	l, -9470(ix)
	ld	h, -9469(ix)
	push	hl
	ld	l, -9468(ix)
	ld	h, -9467(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9472(ix), l
	ld	-9471(ix), h
	ld	l, -9472(ix)
	ld	h, -9471(ix)
	push	hl
	ld	l, -9410(ix)
	ld	h, -9409(ix)
	push	hl
	ld	l, -9372(ix)
	ld	h, -9371(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1899
	dec	sp
	dec	sp
	ld	-9474(ix), l
	ld	-9473(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9476(ix), l
	ld	-9475(ix), h
	ld	l, -9476(ix)
	ld	h, -9475(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9484(ix), l
	ld	-9483(ix), h
	ld	l, -9484(ix)
	ld	h, -9483(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_6071
	ld	hl, #0
	jp	__cmp_e_61002
__cmp_t_6071:
	ld	hl, #1
__cmp_e_61002:
	dec	sp
	dec	sp
	ld	-9486(ix), l
	ld	-9485(ix), h
	ld	l, -9486(ix)
	ld	h, -9485(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_94080
	ld	hl, #0
	jp	__cmp_e_28906
__cmp_t_94080:
	ld	hl, #1
__cmp_e_28906:
	dec	sp
	dec	sp
	ld	-9488(ix), l
	ld	-9487(ix), h
	ld	l, -9488(ix)
	ld	h, -9487(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1903
	jp	__xcc_L1904
__xcc_L1904:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9490(ix), l
	ld	-9489(ix), h
	ld	l, -9490(ix)
	ld	h, -9489(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9498(ix), l
	ld	-9497(ix), h
	ld	l, -9498(ix)
	ld	h, -9497(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9500(ix), l
	ld	-9499(ix), h
	ld	l, -9500(ix)
	ld	h, -9499(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_4485
	ld	hl, #0
	jp	__cmp_e_53491
__cmp_t_4485:
	ld	hl, #1
__cmp_e_53491:
	dec	sp
	dec	sp
	ld	-9502(ix), l
	ld	-9501(ix), h
	ld	l, -9502(ix)
	ld	h, -9501(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_86989
	ld	hl, #0
	jp	__cmp_e_99174
__cmp_t_86989:
	ld	hl, #1
__cmp_e_99174:
	dec	sp
	dec	sp
	ld	-9504(ix), l
	ld	-9503(ix), h
	jp	__xcc_L1905
__xcc_L1903:
	ld	hl, #1
	ld	-9504(ix), l
	ld	-9503(ix), h
__xcc_L1905:
	ld	l, -9504(ix)
	ld	h, -9503(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1900
	jp	__xcc_L1901
__xcc_L1900:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9506(ix), l
	ld	-9505(ix), h
	ld	l, -9506(ix)
	ld	h, -9505(ix)
	dec	sp
	dec	sp
	ld	-9508(ix), l
	ld	-9507(ix), h
	jp	__xcc_L1902
__xcc_L1901:
	ld	hl, #1
	ld	-9508(ix), l
	ld	-9507(ix), h
__xcc_L1902:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9510(ix), l
	ld	-9509(ix), h
	.globl __mul16
	ld	l, -9510(ix)
	ld	h, -9509(ix)
	push	hl
	ld	l, -9508(ix)
	ld	h, -9507(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9512(ix), l
	ld	-9511(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9514(ix), l
	ld	-9513(ix), h
	ld	l, -9514(ix)
	ld	h, -9513(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9522(ix), l
	ld	-9521(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9526(ix), l
	ld	-9525(ix), h
	ld	l, -9522(ix)
	ld	h, -9521(ix)
	push	hl
	ld	l, -9526(ix)
	ld	h, -9525(ix)
	ld	b, l
	pop	hl
__shift_1544:
	ld	a, b
	or	a, a
	jp	z, __sdone_8656
	add	hl, hl
	djnz	__shift_1544
__sdone_8656:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9534(ix), l
	ld	-9533(ix), h
	ld	l, -9534(ix)
	ld	h, -9533(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_98388
	ld	hl, #0
	jp	__cmp_e_60161
__cmp_t_98388:
	ld	hl, #1
__cmp_e_60161:
	dec	sp
	dec	sp
	ld	-9536(ix), l
	ld	-9535(ix), h
	ld	l, -9536(ix)
	ld	h, -9535(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_13970
	ld	hl, #0
	jp	__cmp_e_42738
__cmp_t_13970:
	ld	hl, #1
__cmp_e_42738:
	dec	sp
	dec	sp
	ld	-9538(ix), l
	ld	-9537(ix), h
	ld	l, -9538(ix)
	ld	h, -9537(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1909
	jp	__xcc_L1910
__xcc_L1910:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9540(ix), l
	ld	-9539(ix), h
	ld	l, -9540(ix)
	ld	h, -9539(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9548(ix), l
	ld	-9547(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9552(ix), l
	ld	-9551(ix), h
	ld	l, -9548(ix)
	ld	h, -9547(ix)
	push	hl
	ld	l, -9552(ix)
	ld	h, -9551(ix)
	ld	b, l
	pop	hl
__shift_5841:
	ld	a, b
	or	a, a
	jp	z, __sdone_3102
	add	hl, hl
	djnz	__shift_5841
__sdone_3102:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9560(ix), l
	ld	-9559(ix), h
	ld	l, -9560(ix)
	ld	h, -9559(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9562(ix), l
	ld	-9561(ix), h
	ld	l, -9562(ix)
	ld	h, -9561(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_1379
	ld	hl, #0
	jp	__cmp_e_66848
__cmp_t_1379:
	ld	hl, #1
__cmp_e_66848:
	dec	sp
	dec	sp
	ld	-9564(ix), l
	ld	-9563(ix), h
	ld	l, -9564(ix)
	ld	h, -9563(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_21018
	ld	hl, #0
	jp	__cmp_e_33766
__cmp_t_21018:
	ld	hl, #1
__cmp_e_33766:
	dec	sp
	dec	sp
	ld	-9566(ix), l
	ld	-9565(ix), h
	jp	__xcc_L1911
__xcc_L1909:
	ld	hl, #1
	ld	-9566(ix), l
	ld	-9565(ix), h
__xcc_L1911:
	ld	l, -9566(ix)
	ld	h, -9565(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1906
	jp	__xcc_L1907
__xcc_L1906:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9568(ix), l
	ld	-9567(ix), h
	ld	l, -9568(ix)
	ld	h, -9567(ix)
	dec	sp
	dec	sp
	ld	-9570(ix), l
	ld	-9569(ix), h
	jp	__xcc_L1908
__xcc_L1907:
	ld	hl, #1
	ld	-9570(ix), l
	ld	-9569(ix), h
__xcc_L1908:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9572(ix), l
	ld	-9571(ix), h
	.globl __mul16
	ld	l, -9572(ix)
	ld	h, -9571(ix)
	push	hl
	ld	l, -9570(ix)
	ld	h, -9569(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9574(ix), l
	ld	-9573(ix), h
	ld	l, -9574(ix)
	ld	h, -9573(ix)
	push	hl
	ld	l, -9512(ix)
	ld	h, -9511(ix)
	push	hl
	ld	l, -9474(ix)
	ld	h, -9473(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1884:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1883
	jp	__xcc_L1885
__xcc_L1885:
__xcc_L1912:
	ld	hl, #__str_1915
	dec	sp
	dec	sp
	ld	-9576(ix), l
	ld	-9575(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9578(ix), l
	ld	-9577(ix), h
	ld	l, -9578(ix)
	ld	h, -9577(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9586(ix), l
	ld	-9585(ix), h
	ld	l, -9586(ix)
	ld	h, -9585(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_838
	ld	hl, #0
	jp	__cmp_e_54042
__cmp_t_838:
	ld	hl, #1
__cmp_e_54042:
	dec	sp
	dec	sp
	ld	-9588(ix), l
	ld	-9587(ix), h
	ld	l, -9588(ix)
	ld	h, -9587(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_50250
	ld	hl, #0
	jp	__cmp_e_2942
__cmp_t_50250:
	ld	hl, #1
__cmp_e_2942:
	dec	sp
	dec	sp
	ld	-9590(ix), l
	ld	-9589(ix), h
	ld	l, -9590(ix)
	ld	h, -9589(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1919
	jp	__xcc_L1920
__xcc_L1920:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9592(ix), l
	ld	-9591(ix), h
	ld	l, -9592(ix)
	ld	h, -9591(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9600(ix), l
	ld	-9599(ix), h
	ld	l, -9600(ix)
	ld	h, -9599(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9602(ix), l
	ld	-9601(ix), h
	ld	l, -9602(ix)
	ld	h, -9601(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_84284
	ld	hl, #0
	jp	__cmp_e_30959
__cmp_t_84284:
	ld	hl, #1
__cmp_e_30959:
	dec	sp
	dec	sp
	ld	-9604(ix), l
	ld	-9603(ix), h
	ld	l, -9604(ix)
	ld	h, -9603(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_8488
	ld	hl, #0
	jp	__cmp_e_54461
__cmp_t_8488:
	ld	hl, #1
__cmp_e_54461:
	dec	sp
	dec	sp
	ld	-9606(ix), l
	ld	-9605(ix), h
	jp	__xcc_L1921
__xcc_L1919:
	ld	hl, #1
	ld	-9606(ix), l
	ld	-9605(ix), h
__xcc_L1921:
	ld	l, -9606(ix)
	ld	h, -9605(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1916
	jp	__xcc_L1917
__xcc_L1916:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9608(ix), l
	ld	-9607(ix), h
	ld	l, -9608(ix)
	ld	h, -9607(ix)
	dec	sp
	dec	sp
	ld	-9610(ix), l
	ld	-9609(ix), h
	jp	__xcc_L1918
__xcc_L1917:
	ld	hl, #1
	ld	-9610(ix), l
	ld	-9609(ix), h
__xcc_L1918:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9612(ix), l
	ld	-9611(ix), h
	.globl __mul16
	ld	l, -9612(ix)
	ld	h, -9611(ix)
	push	hl
	ld	l, -9610(ix)
	ld	h, -9609(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9614(ix), l
	ld	-9613(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9616(ix), l
	ld	-9615(ix), h
	ld	l, -9616(ix)
	ld	h, -9615(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9624(ix), l
	ld	-9623(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9632(ix), l
	ld	-9631(ix), h
	ld	l, -9624(ix)
	ld	h, -9623(ix)
	push	hl
	ld	l, -9632(ix)
	ld	h, -9631(ix)
	ld	b, l
	pop	hl
__shift_5135:
	ld	a, b
	or	a, a
	jp	z, __sdone_2596
	add	hl, hl
	djnz	__shift_5135
__sdone_2596:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9640(ix), l
	ld	-9639(ix), h
	ld	l, -9640(ix)
	ld	h, -9639(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_88924
	ld	hl, #0
	jp	__cmp_e_71206
__cmp_t_88924:
	ld	hl, #1
__cmp_e_71206:
	dec	sp
	dec	sp
	ld	-9642(ix), l
	ld	-9641(ix), h
	ld	l, -9642(ix)
	ld	h, -9641(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_83598
	ld	hl, #0
	jp	__cmp_e_99356
__cmp_t_83598:
	ld	hl, #1
__cmp_e_99356:
	dec	sp
	dec	sp
	ld	-9644(ix), l
	ld	-9643(ix), h
	ld	l, -9644(ix)
	ld	h, -9643(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1925
	jp	__xcc_L1926
__xcc_L1926:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9646(ix), l
	ld	-9645(ix), h
	ld	l, -9646(ix)
	ld	h, -9645(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9654(ix), l
	ld	-9653(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9662(ix), l
	ld	-9661(ix), h
	ld	l, -9654(ix)
	ld	h, -9653(ix)
	push	hl
	ld	l, -9662(ix)
	ld	h, -9661(ix)
	ld	b, l
	pop	hl
__shift_113:
	ld	a, b
	or	a, a
	jp	z, __sdone_4435
	add	hl, hl
	djnz	__shift_113
__sdone_4435:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9670(ix), l
	ld	-9669(ix), h
	ld	l, -9670(ix)
	ld	h, -9669(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9672(ix), l
	ld	-9671(ix), h
	ld	l, -9672(ix)
	ld	h, -9671(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_52847
	ld	hl, #0
	jp	__cmp_e_3454
__cmp_t_52847:
	ld	hl, #1
__cmp_e_3454:
	dec	sp
	dec	sp
	ld	-9674(ix), l
	ld	-9673(ix), h
	ld	l, -9674(ix)
	ld	h, -9673(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_19962
	ld	hl, #0
	jp	__cmp_e_80744
__cmp_t_19962:
	ld	hl, #1
__cmp_e_80744:
	dec	sp
	dec	sp
	ld	-9676(ix), l
	ld	-9675(ix), h
	jp	__xcc_L1927
__xcc_L1925:
	ld	hl, #1
	ld	-9676(ix), l
	ld	-9675(ix), h
__xcc_L1927:
	ld	l, -9676(ix)
	ld	h, -9675(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1922
	jp	__xcc_L1923
__xcc_L1922:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9678(ix), l
	ld	-9677(ix), h
	ld	l, -9678(ix)
	ld	h, -9677(ix)
	dec	sp
	dec	sp
	ld	-9680(ix), l
	ld	-9679(ix), h
	jp	__xcc_L1924
__xcc_L1923:
	ld	hl, #1
	ld	-9680(ix), l
	ld	-9679(ix), h
__xcc_L1924:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9682(ix), l
	ld	-9681(ix), h
	.globl __mul16
	ld	l, -9682(ix)
	ld	h, -9681(ix)
	push	hl
	ld	l, -9680(ix)
	ld	h, -9679(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9684(ix), l
	ld	-9683(ix), h
	ld	l, -9684(ix)
	ld	h, -9683(ix)
	push	hl
	ld	l, -9614(ix)
	ld	h, -9613(ix)
	push	hl
	ld	l, -9576(ix)
	ld	h, -9575(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_1928
	dec	sp
	dec	sp
	ld	-9686(ix), l
	ld	-9685(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9688(ix), l
	ld	-9687(ix), h
	ld	l, -9688(ix)
	ld	h, -9687(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9696(ix), l
	ld	-9695(ix), h
	ld	l, -9696(ix)
	ld	h, -9695(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_32110
	ld	hl, #0
	jp	__cmp_e_34702
__cmp_t_32110:
	ld	hl, #1
__cmp_e_34702:
	dec	sp
	dec	sp
	ld	-9698(ix), l
	ld	-9697(ix), h
	ld	l, -9698(ix)
	ld	h, -9697(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_57257
	ld	hl, #0
	jp	__cmp_e_62432
__cmp_t_57257:
	ld	hl, #1
__cmp_e_62432:
	dec	sp
	dec	sp
	ld	-9700(ix), l
	ld	-9699(ix), h
	ld	l, -9700(ix)
	ld	h, -9699(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1932
	jp	__xcc_L1933
__xcc_L1933:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9702(ix), l
	ld	-9701(ix), h
	ld	l, -9702(ix)
	ld	h, -9701(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9710(ix), l
	ld	-9709(ix), h
	ld	l, -9710(ix)
	ld	h, -9709(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9712(ix), l
	ld	-9711(ix), h
	ld	l, -9712(ix)
	ld	h, -9711(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_77440
	ld	hl, #0
	jp	__cmp_e_69451
__cmp_t_77440:
	ld	hl, #1
__cmp_e_69451:
	dec	sp
	dec	sp
	ld	-9714(ix), l
	ld	-9713(ix), h
	ld	l, -9714(ix)
	ld	h, -9713(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_11886
	ld	hl, #0
	jp	__cmp_e_95171
__cmp_t_11886:
	ld	hl, #1
__cmp_e_95171:
	dec	sp
	dec	sp
	ld	-9716(ix), l
	ld	-9715(ix), h
	jp	__xcc_L1934
__xcc_L1932:
	ld	hl, #1
	ld	-9716(ix), l
	ld	-9715(ix), h
__xcc_L1934:
	ld	l, -9716(ix)
	ld	h, -9715(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1929
	jp	__xcc_L1930
__xcc_L1929:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9718(ix), l
	ld	-9717(ix), h
	ld	l, -9718(ix)
	ld	h, -9717(ix)
	dec	sp
	dec	sp
	ld	-9720(ix), l
	ld	-9719(ix), h
	jp	__xcc_L1931
__xcc_L1930:
	ld	hl, #1
	ld	-9720(ix), l
	ld	-9719(ix), h
__xcc_L1931:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9722(ix), l
	ld	-9721(ix), h
	.globl __mul16
	ld	l, -9722(ix)
	ld	h, -9721(ix)
	push	hl
	ld	l, -9720(ix)
	ld	h, -9719(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9724(ix), l
	ld	-9723(ix), h
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9726(ix), l
	ld	-9725(ix), h
	ld	l, -9726(ix)
	ld	h, -9725(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9734(ix), l
	ld	-9733(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9742(ix), l
	ld	-9741(ix), h
	ld	l, -9734(ix)
	ld	h, -9733(ix)
	push	hl
	ld	l, -9742(ix)
	ld	h, -9741(ix)
	ld	b, l
	pop	hl
__shift_6299:
	ld	a, b
	or	a, a
	jp	z, __sdone_2904
	add	hl, hl
	djnz	__shift_6299
__sdone_2904:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9750(ix), l
	ld	-9749(ix), h
	ld	l, -9750(ix)
	ld	h, -9749(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_45289
	ld	hl, #0
	jp	__cmp_e_53489
__cmp_t_45289:
	ld	hl, #1
__cmp_e_53489:
	dec	sp
	dec	sp
	ld	-9752(ix), l
	ld	-9751(ix), h
	ld	l, -9752(ix)
	ld	h, -9751(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_86946
	ld	hl, #0
	jp	__cmp_e_95539
__cmp_t_86946:
	ld	hl, #1
__cmp_e_95539:
	dec	sp
	dec	sp
	ld	-9754(ix), l
	ld	-9753(ix), h
	ld	l, -9754(ix)
	ld	h, -9753(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1938
	jp	__xcc_L1939
__xcc_L1939:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9756(ix), l
	ld	-9755(ix), h
	ld	l, -9756(ix)
	ld	h, -9755(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9764(ix), l
	ld	-9763(ix), h
	ld	hl, #1
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9772(ix), l
	ld	-9771(ix), h
	ld	l, -9764(ix)
	ld	h, -9763(ix)
	push	hl
	ld	l, -9772(ix)
	ld	h, -9771(ix)
	ld	b, l
	pop	hl
__shift_2784:
	ld	a, b
	or	a, a
	jp	z, __sdone_7582
	add	hl, hl
	djnz	__shift_2784
__sdone_7582:
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-9780(ix), l
	ld	-9779(ix), h
	ld	l, -9780(ix)
	ld	h, -9779(ix)
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9782(ix), l
	ld	-9781(ix), h
	ld	l, -9782(ix)
	ld	h, -9781(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_42850
	ld	hl, #0
	jp	__cmp_e_81272
__cmp_t_42850:
	ld	hl, #1
__cmp_e_81272:
	dec	sp
	dec	sp
	ld	-9784(ix), l
	ld	-9783(ix), h
	ld	l, -9784(ix)
	ld	h, -9783(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_42043
	ld	hl, #0
	jp	__cmp_e_7986
__cmp_t_42043:
	ld	hl, #1
__cmp_e_7986:
	dec	sp
	dec	sp
	ld	-9786(ix), l
	ld	-9785(ix), h
	jp	__xcc_L1940
__xcc_L1938:
	ld	hl, #1
	ld	-9786(ix), l
	ld	-9785(ix), h
__xcc_L1940:
	ld	l, -9786(ix)
	ld	h, -9785(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1935
	jp	__xcc_L1936
__xcc_L1935:
	ld	hl, #1
	ld	a, l
	cpl
	ld	l, a
	ld	a, h
	cpl
	ld	h, a
	inc	hl
	dec	sp
	dec	sp
	ld	-9788(ix), l
	ld	-9787(ix), h
	ld	l, -9788(ix)
	ld	h, -9787(ix)
	dec	sp
	dec	sp
	ld	-9790(ix), l
	ld	-9789(ix), h
	jp	__xcc_L1937
__xcc_L1936:
	ld	hl, #1
	ld	-9790(ix), l
	ld	-9789(ix), h
__xcc_L1937:
	ld	hl, #8
	dec	sp
	dec	sp
	ld	-9792(ix), l
	ld	-9791(ix), h
	.globl __mul16
	ld	l, -9792(ix)
	ld	h, -9791(ix)
	push	hl
	ld	l, -9790(ix)
	ld	h, -9789(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9794(ix), l
	ld	-9793(ix), h
	ld	l, -9794(ix)
	ld	h, -9793(ix)
	push	hl
	ld	l, -9724(ix)
	ld	h, -9723(ix)
	push	hl
	ld	l, -9686(ix)
	ld	h, -9685(ix)
	push	hl
	.globl _check
	call	_check
	pop	bc
	pop	bc
	pop	bc
__xcc_L1913:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1912
	jp	__xcc_L1914
__xcc_L1914:
__xcc_L1823:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1822
	jp	__xcc_L1824
__xcc_L1824:
__xcc_L1701:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1700
	jp	__xcc_L1702
__xcc_L1702:
__xcc_L975:
	ld	hl, #0
	ld	a, h
	or	a, l
	jp	nz, __xcc_L974
	jp	__xcc_L976
__xcc_L976:
	ld	hl, #__str_1941
	dec	sp
	dec	sp
	ld	-9796(ix), l
	ld	-9795(ix), h
	ld	hl, (_nfailed)
	push	hl
	ld	l, -9796(ix)
	ld	h, -9795(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-9798(ix), l
	ld	-9797(ix), h
	ld	hl, (_nfailed)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_3868
	ld	hl, #0
	jp	__cmp_e_47319
__cmp_t_3868:
	ld	hl, #1
__cmp_e_47319:
	dec	sp
	dec	sp
	ld	-9800(ix), l
	ld	-9799(ix), h
	ld	l, -9800(ix)
	ld	h, -9799(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
