import optparse

parser = optparse.OptionParser()
parser.add_option("-b", "--benchmark", type="string", default="",
                  help="The SPEC benchmark script to be generated."
                       "Benchmarks:  perlbench gcc bwaves mcf cactuBSSN "
                       "lbm omnetpp wrf xalancbmk x264 cam4 pop2 deepsjeng "
                       "imagick leela nab exchange2 fotonik3d roms xz")

(options, args) = parser.parse_args()

if options.benchmark:
    if options.benchmark == 'perlbench':
        benchmark = '600.perlbench_s'
        command = ('../../bin/600.perlbench_s -I./lib checkspam.pl '
                   '2500 5 25 11 150 1 1 1 1'
                   ' > checkspam.2500.5.25.11.150.1.1.1.1.out'
                   ' 2>> checkspam.2500.5.25.11.150.1.1.1.1.err')
    elif options.benchmark == 'gcc':
        benchmark = '602.gcc_s'
        command = ('../../bin/602.gcc_s gcc-pp.c -O5 -fipa-pta'
                   ' -o gcc-pp.opts-O5_-fipa-pta.s'
                   ' > gcc-pp.opts-O5_-fipa-pta.out'
                   ' 2>> gcc-pp.opts-O5_-fipa-pta.err')
    elif options.benchmark == 'bwaves':
        benchmark = '603.bwaves_s'
        command = ('../../bin/603.bwaves_s bwaves_1 < bwaves_1.in'
                   ' > bwaves_1.out 2>> bwaves_1.err')
    elif options.benchmark == 'mcf':
        benchmark = '605.mcf_s'
        command = '../../bin/605.mcf_s inp.in  > inp.out 2>> inp.err'
    elif options.benchmark == 'cactuBSSN':
        benchmark = '607.cactuBSSN_s'
        command = ('../../bin/607.cactuBSSN_s spec_ref.par '
                   '> spec_ref.out 2>> spec_ref.err')
    elif options.benchmark == 'lbm':
        benchmark = '619.lbm_s'
        command = ('../../bin/619.lbm_s 2000 reference.dat '
                   '0 0 200_200_260_ldc.of > lbm.out 2>> lbm.err')
    elif options.benchmark == 'omnetpp':
        benchmark = '620.omnetpp_s'
        command = ('../../bin/620.omnetpp_s -c General -r 0 '
                   '> omnetpp.General-0.out 2>> omnetpp.General-0.err')
    elif options.benchmark == 'wrf':
        benchmark = '621.wrf_s'
        command = '../../bin/621.wrf_s > rsl.out.0000 2>> wrf.err'
    elif options.benchmark == 'xalancbmk':
        benchmark = '623.xalancbmk_s'
        command = ('../../bin/623.xalancbmk_s -v t5.xml xalanc.xsl '
                   '> ref-t5.out 2>> ref-t5.err')
    elif options.benchmark == 'x264':
        benchmark = '625.x264_s'
        command = ('../../bin/625.x264_s --seek 500 --dumpyuv 200 --frames '
                   '1250 -o BuckBunny_New.264 BuckBunny.yuv 1280x720 '
                   '> run_0500-1250_x264_s.out 2>> run_0500-1250_x264_s.err')
    elif options.benchmark == 'cam4':
        benchmark = '627.cam4_s'
        command = '../../bin/627.cam4_s > cam4_s.txt 2>> cam4_s.err'
    elif options.benchmark == 'pop2':
        benchmark = '628.pop2_s'
        command = '../../bin/628.pop2_s > pop2_s.out 2>> pop2_s.err'
    elif options.benchmark == 'deepsjeng':
        benchmark = '631.deepsjeng_s'
        command = '../../bin/631.deepsjeng_s ref.txt > ref.out 2>> ref.err'
    elif options.benchmark == 'imagick':
        benchmark = '638.imagick_s'
        command = ('../../bin/638.imagick_s -limit disk 0 refspeed_input.tga '
                   '-resize 817% -rotate -2.76 -shave 540x375 -alpha remove '
                   '-auto-level -contrast-stretch 1x1% -colorspace Lab '
                   '-channel R -equalize +channel -colorspace sRGB -define '
                   'histogram:unique-colors=false -adaptive-blur 0x5 '
                   '-despeckle -auto-gamma -adaptive-sharpen 55 -enhance '
                   '-brightness-contrast 10x10 -resize 30% '
                   'refspeed_output.tga > refspeed_convert.out '
                   '2>> refspeed_convert.err')
    elif options.benchmark == 'leela':
        benchmark = '641.leela_s'
        command = '../../bin/641.leela_s ref.sgf > ref.out 2>> ref.err'
    elif options.benchmark == 'nab':
        benchmark = '644.nab_s'
        command = ('../../bin/644.nab_s 3j1n 20140317 220 > 3j1n.out '
                   '2>> 3j1n.err')
    elif options.benchmark == 'exchange2':
        benchmark = '648.exchange2_s'
        command = ('../../bin/648.exchange2_s 6 > exchange2.txt '
                   '2>> exchange2.err')
    elif options.benchmark == 'fotonik3d':
        benchmark = '649.fotonik3d_s'
        command = ('../../bin/649.fotonik3d_s > fotonik3d_s.log '
                   '2>> fotonik3d_s.err')
    elif options.benchmark == 'roms':
        benchmark = '654.roms_s'
        command = ('../../bin/664.roms_s < ocean_benchmark3.in '
                   '> ocean_benchmark3.log 2>> ocean_benchmark3.err')
    elif options.benchmark == 'xz':
        benchmark = '657.xz_s'
        command = ('../../bin/657.xz_s 1400 '
                   '19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c'
                   '13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd545'
                   '4852652806a657bb56e07d61128434b474 536995164 '
                   '539938872 8 > cld.tar-1400-8.out 2>> cld.tar-1400-8.err')
    else:
        print "No recognized SPEC CPU2017 benchmark selected! Exiting."
        sys.exit(1)
else:
    print >> sys.stderr, "Need --benchmark switch to specify SPEC \
                          CPU2017 workload. Exiting!\n"

#print('benchmark: ' + benchmark)
#print('command: ' + command)

scriptfile = open(benchmark + '.rcS', 'w')

scriptfile.write('#!/bin/sh\n\n')
scriptfile.write('# File to run the ' + benchmark + ' benchmark\n\n')
scriptfile.write('cd cpu2017/run/' + benchmark + '\n')
scriptfile.write('echo "Start ..."\n')
scriptfile.write('/sbin/m5 dumpstats\n')
scriptfile.write('/sbin/m5 resetstats\n')
scriptfile.write(command + '\n')
scriptfile.write('echo "Done :D"\n')
scriptfile.write('/sbin/m5 exit')

scriptfile.close()
