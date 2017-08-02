_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f objfre_w2k_x86\i386\viostor.inf -d * -a $(_BUILDARCH).$(_NT_TARGET_MAJ_ARCH).$(_NT_TARGET_MIN_ARCH)


$(INF_TARGET): $(_INX)\$(INF_NAME).inx
    copy $(_INX)\$(INF_NAME).inx objfre_w2k_x86\i386\viostor.inf
    $(STAMP)


