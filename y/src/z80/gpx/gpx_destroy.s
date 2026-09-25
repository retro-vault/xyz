        ;; Release an independently allocated GPX context.
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_destroy
        .optsdcc -mz80 sdcccall(1)
	.globl  _gpx_destroy
	.globl  __os_free
	.area   _CODE
	;; HL = context (NULL permitted); no result.
	;; Clobbers AF, BC, DE, HL; preserves IX/IY.
_gpx_destroy::
	jp      __os_free
