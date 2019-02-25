project('yoshimi', 'cpp',
  version : '1.5.10.1',
  default_options : ['warning_level=3',
                     'cpp_std=gnu++11'])
cpp = meson.get_compiler('cpp')

add_project_arguments('-DDEFAULT_AUDIO=jack_audio', language : 'cpp')
add_project_arguments('-DDEFAULT_MIDI=jack_midi', language : 'cpp')
add_project_arguments('-DGUI_FLTK', language : 'cpp')
add_project_arguments('-DHAVE_ALSA', language : 'cpp')
add_project_arguments('-DJACK_LATENCY', language : 'cpp')
add_project_arguments('-DJACK_SESSION', language : 'cpp')
add_project_arguments('-DMIN_CONFIG_MAJOR=1', language : 'cpp')
add_project_arguments('-DMIN_CONFIG_MINOR=5', language : 'cpp')
add_project_arguments('-DYOSHIMI="yoshimi"', language : 'cpp')
add_project_arguments('-DYOSHIMI_VERSION="1.5.10 M"', language : 'cpp')
add_project_arguments('-DYOSHI_FIFO_DIR=""', language : 'cpp')

alsa_dep = dependency('alsa')
cairo_dep = dependency('cairo')
fftw3f_dep = dependency('fftw3f')
fltk_dep = cpp.find_library('fltk')
fltk_images_dep = cpp.find_library('fltk_images')
jack_dep = dependency('jack')
mxml_dep = dependency('mxml')
ncurses_dep = dependency('ncurses')
readline_dep = cpp.find_library('readline')
thread_dep = dependency('threads')
zlib_dep = dependency('zlib')

fluid = generator(find_program('fluid'),
  output : ['@BASENAME@.cpp', '@BASENAME@.h'],
  arguments : ['-c', '-o', '@OUTPUT0@', '-h', '@OUTPUT1@', '@INPUT@'])

fl_sources = fluid.process('src/UI/ADnoteUI.fl', 'src/UI/BankUI.fl',
  'src/UI/ConfigUI.fl', 'src/UI/ConsoleUI.fl', 'src/UI/EffUI.fl',
  'src/UI/EnvelopeUI.fl', 'src/UI/FilterUI.fl', 'src/UI/LFOUI.fl',
  'src/UI/MasterMiscUI.fl', 'src/UI/MasterUI.fl', 'src/UI/MicrotonalUI.fl',
  'src/UI/MidiLearnUI.fl', 'src/UI/OscilGenUI.fl', 'src/UI/PADnoteUI.fl',
  'src/UI/ParametersUI.fl', 'src/UI/PartUI.fl', 'src/UI/PresetsUI.fl',
  'src/UI/ResonanceUI.fl', 'src/UI/SUBnoteUI.fl', 'src/UI/VectorUI.fl',
  'src/UI/VirKeyboardUI.fl')

sources = [
  'src/DSP/AnalogFilter.cpp',
  'src/DSP/FFTwrapper.cpp',
  'src/DSP/Filter.cpp',
  'src/DSP/FormantFilter.cpp',
  'src/DSP/SVFilter.cpp',
  'src/DSP/Unison.cpp',
  'src/Effects/Alienwah.cpp',
  'src/Effects/Chorus.cpp',
  'src/Effects/Distorsion.cpp',
  'src/Effects/DynamicFilter.cpp',
  'src/Effects/EQ.cpp',
  'src/Effects/Echo.cpp',
  'src/Effects/Effect.cpp',
  'src/Effects/EffectLFO.cpp',
  'src/Effects/EffectMgr.cpp',
  'src/Effects/Phaser.cpp',
  'src/Effects/Reverb.cpp',
  'src/Interface/CmdInterface.cpp',
  'src/Interface/FileMgr.cpp',
  'src/Interface/InterChange.cpp',
  'src/Interface/MidiDecode.cpp',
  'src/Interface/MidiLearn.cpp',
  'src/Misc/Bank.cpp',
  'src/Misc/Config.cpp',
  'src/Misc/Microtonal.cpp',
  'src/Misc/MiscFuncs.cpp',
  'src/Misc/Part.cpp',
  'src/Misc/Splash.cpp',
  'src/Misc/SynthEngine.cpp',
  'src/Misc/WavFile.cpp',
  'src/Misc/XMLwrapper.cpp',
  'src/MusicIO/AlsaEngine.cpp',
  'src/MusicIO/JackEngine.cpp',
  'src/MusicIO/MusicClient.cpp',
  'src/MusicIO/MusicIO.cpp',
  'src/Params/ADnoteParameters.cpp',
  'src/Params/Controller.cpp',
  'src/Params/EnvelopeParams.cpp',
  'src/Params/FilterParams.cpp',
  'src/Params/LFOParams.cpp',
  'src/Params/PADnoteParameters.cpp',
  'src/Params/Presets.cpp',
  'src/Params/PresetsStore.cpp',
  'src/Params/SUBnoteParameters.cpp',
  'src/Params/UnifiedPresets.cpp',
  'src/Synth/ADnote.cpp',
  'src/Synth/Envelope.cpp',
  'src/Synth/LFO.cpp',
  'src/Synth/OscilGen.cpp',
  'src/Synth/PADnote.cpp',
  'src/Synth/Resonance.cpp',
  'src/Synth/SUBnote.cpp',
  'src/UI/DynamicTooltip.cpp',
  'src/UI/MiscGui.cpp',
  'src/UI/WidgetMWSlider.cpp',
  'src/UI/WidgetPDial.cpp',
  'src/main.cpp',
]

exe = executable('yoshimi', sources, fl_sources,
  include_directories : 'src',
  dependencies : [alsa_dep, cairo_dep, fftw3f_dep, fltk_dep, fltk_images_dep,
    jack_dep, mxml_dep, ncurses_dep, readline_dep, thread_dep, zlib_dep],
  install : true)
