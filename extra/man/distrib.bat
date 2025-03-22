@echo off
tar --directory=. --exclude=distrib --exclude=*.chm --exclude=BuildLog.htm --exclude=*.suo --exclude=*.sln --exclude=*.ncb --exclude=*.vcproj --exclude=pe.cfg --exclude=*.bat --exclude=*.bak --exclude=*.hhc --exclude=*.hhp --exclude=*.hhk --exclude=*.gz --exclude=*.zip --exclude=*.sq --exclude=arv --exclude=*.user -cvf - . | gzip --best  > lp_solve_5.5_doc.tar.gz
