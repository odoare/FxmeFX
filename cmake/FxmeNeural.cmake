# ─────────────────────────────────────────────────────────────────────────────
# FxmeNeural.cmake
#
# fxmefx_attach_neural(<target>)
#
# What a target compiling the NeuralAmp effect (Source/NeuralAmp) needs on
# top of fxmetools_attach(): FxmeTools' FxmeNeural target (neural amp
# models, JUCE-free) and its backend, NeuralAudio. For FxmeFX's own plugin
# and for every host that compiles FxmeFX effects from source (Otomato,
# Camshaft, Mechanodd).
#
# The top-level project pins NeuralAudio, as FxmeFX does in
# cmake/Dependencies.cmake:
#     fxme_dependency(NeuralAudio https://github.com/odoare/NeuralAudio <sha>)
#
# The networks are built for x86-64-v3 (AVX2, FMA: CPUs from 2013 on; they
# cost half as much as on the x86-64 baseline), decided 2026-11-10. On a
# CPU without them the effect loads no model and says why, instead of
# crashing. -DFXME_NEURAL_ARCH= (empty) builds for any x86-64 CPU. ARM
# builds (Apple silicon's slice, a Raspberry Pi) are not affected.
#
# macOS: NeuralAudio needs 10.15 (std::filesystem), so a plugin with this
# effect is a 10.15 plugin, whatever CMAKE_OSX_DEPLOYMENT_TARGET says.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

set(FXME_NEURAL_ARCH "x86-64-v3" CACHE STRING "-march for the neural networks: empty, x86-64-v3 or native")

function(fxmefx_attach_neural target)
    if(NOT COMMAND fxmetools_attach_neural)
        message(FATAL_ERROR "fxmefx_attach_neural(${target}): include FxmeTools.cmake first")
    endif()
    fxmetools_attach_neural(${target})
endfunction()
