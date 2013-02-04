#!/usr/bin/env ruby
# coding: utf-8


require 'find'
require 'pathname'


def exec(cmd)
    puts(cmd)
    exit 1 unless system(cmd)
end


if ARGV[0] == 'clean'
    exec("rm -f xemu child obj/*.o")
    exit 0
end


incdirs = ['include']

cc = 'gcc'
cflags = "-std=c11 -O3 -g2 -Wall -Wextra #{incdirs.map { |d| "'-I#{d}'" } * ' '} #{`sdl-config --cflags`.chomp}"
cld = 'gcc'
cldflags = ''
asm = 'fasm'
asmflags = ''
asmout = ''
asmquiet = true
asmld = 'ld'
asmldflags = '-e _start -Ttext 0xFFFFC000 -Tbss 0xFFFFC800 -melf_i386'


system('mkdir -p obj')


objs = []


Find.find('src') do |path|
    next unless File.file?(path)

    path = Pathname.new(path).cleanpath.to_s


    obj = "#{Dir.pwd}/obj/#{path.gsub('/', '__')}.o"
    objs << obj

    next if File.file?(obj) && (File.mtime(obj) > File.mtime(path))


    case File.extname(path)
    when '.c'
        exec("#{cc} #{cflags} -c '#{path}' -o '#{obj}'")
    when '.asm'
        exec("#{asm} #{asmflags} '#{path}' #{asmout} '#{obj}' #{asmquiet ? '&> /dev/null' : ''}")
    end
end


child = 'src__child__child.asm.o'

exec("#{cld} #{cldflags} #{objs.reject { |o| File.basename(o) == child }.map { |o| "'#{o}'" } * ' '} -o xemu -lrt -lpthread #{`sdl-config --cflags --libs`.gsub("\n", ' ')}")

exec("#{asmld} #{asmldflags} obj/#{child} -o child")
