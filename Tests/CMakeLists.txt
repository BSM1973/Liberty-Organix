add_executable(organix_dsp_smoke dsp_smoke.cpp ../Source/MaterialDSP.cpp)
target_compile_features(organix_dsp_smoke PRIVATE cxx_std_17)
add_test(NAME organix_dsp_smoke COMMAND organix_dsp_smoke)
add_executable(organix_preset_registry preset_registry.cpp)
target_compile_features(organix_preset_registry PRIVATE cxx_std_17)
add_test(NAME organix_preset_registry COMMAND organix_preset_registry)

add_executable(organix_material_controls material_controls.cpp ../Source/MaterialDSP.cpp)
target_compile_features(organix_material_controls PRIVATE cxx_std_17)
add_test(NAME organix_material_controls COMMAND organix_material_controls)

add_executable(organix_ui_geometry ui_geometry.cpp)
target_compile_features(organix_ui_geometry PRIVATE cxx_std_17)
add_test(NAME organix_ui_geometry COMMAND organix_ui_geometry)

add_executable(organix_motion_width motion_width.cpp)
target_compile_features(organix_motion_width PRIVATE cxx_std_17)
add_test(NAME organix_motion_width COMMAND organix_motion_width)

add_executable(organix_matter_dna matter_dna.cpp ../Source/MatterDNA.cpp)
target_compile_features(organix_matter_dna PRIVATE cxx_std_17)
add_test(NAME organix_matter_dna COMMAND organix_matter_dna)

add_executable(organix_morph_4d morph_4d.cpp ../Source/OrganixMorph4D.cpp ../Source/OrganixModMatrix.cpp ../Source/MaterialReactions.cpp ../Source/MaterialDSP.cpp)
target_compile_features(organix_morph_4d PRIVATE cxx_std_17)
add_test(NAME organix_morph_4d COMMAND organix_morph_4d)

add_executable(organix_trajectory_designer trajectory_designer.cpp ../Source/OrganixTrajectory.cpp ../Source/OrganixMorph4D.cpp ../Source/OrganixModMatrix.cpp ../Source/MaterialReactions.cpp ../Source/MaterialDSP.cpp)
target_compile_features(organix_trajectory_designer PRIVATE cxx_std_17)
add_test(NAME organix_trajectory_designer COMMAND organix_trajectory_designer)

add_executable(organix_material_reactions material_reactions.cpp ../Source/MaterialReactions.cpp ../Source/MaterialDSP.cpp)
target_compile_features(organix_material_reactions PRIVATE cxx_std_17)
add_test(NAME organix_material_reactions COMMAND organix_material_reactions)

add_executable(organix_modulation_matrix modulation_matrix.cpp ../Source/OrganixModMatrix.cpp)
target_compile_features(organix_modulation_matrix PRIVATE cxx_std_17)
add_test(NAME organix_modulation_matrix COMMAND organix_modulation_matrix)
