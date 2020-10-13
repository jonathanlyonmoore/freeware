 $!A $! This command file sets up DCL symbols for commonly-used tools.  $!D $! This procedure determines which images are needed for the currentF $! system architecture (VAX, Alpha or IA-64), and sets up the symbols.@ $! Not all tools are necessarily available on all architectures. $!$ $ Arch       = f$getsyi("ARCH_NAME")= $ Device     = f$parse(f$environment("PROCEDURE"),,,"DEVICE") @ $ Direct     = f$parse(f$environment("PROCEDURE"),,,"DIRECTORY"); $ Direct     = Direct - "][" - "><" - ">" - "<" - "]" - "[" * $ DevDir     = Device + "[" + Direct + "]"> $ DevDirArch = Device + "[" + Direct + "." + Arch + "_IMAGES]" $ K $ BZIP       :== $'DevDirArch'BZIP2.exe        ! BZip2 (De)Compression Tool K $ D64        :== $'DevDirArch'DECODE_64.exe    ! decode base64 encoded file 8 $ GZIP       :== $'DevDirArch'GZIP.exe         ! Gnu ZipF $ MXRN       :== $'DevDirArch'MXRN.exe         ! Motif XRN News ReaderI $ UNZI*P     :== $'DevDirArch'UNZIP.exe        ! Unzip Decompression Tool @ $ VMST*AR    :== $'DevDirArch'VMSTAR.exe       ! VMS Tar UtilityE $ ZIP        :== $'DevDirArch'ZIP.exe          ! Zip Compression Tool  $ B $ FIXBCK     :== @'DevDir'RESET_BACKUP_SAVESET_FILE_ATTRIBUTES.COM $  $ Exit