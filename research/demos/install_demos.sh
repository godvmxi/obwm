#!/bin/bash
QMAKE=qmake-qt4
MAKE=make
install_demos()
{
	cd desktop
	$QMAKE
	make
	cp desktop /bin/ -rf
	make distclean
	cd ../app
	$QMAKE
	make
	cp app /bin/ -rf
	make distclean
	cd ../home
	$QMAKE
	make
	cp home /bin/ -rf
	make distclean
	cd ..
}
remove_demos()
{
	rm -rf /bin/desktop /bin/app /bin/home
	echo "clean demos"
}
show_help()
{
	cat <<SHOW
USAGE: 
install

SHOW

}
while true; do
	case "$1" in
		--install)
		install_demos
		break
		;;
		--remove)
		remove_demos
		break;
		;;
		--help)
		show_help
		break
		;;
		*)
		remove_demos
		install_demos
		break
		;;
	esac
done



