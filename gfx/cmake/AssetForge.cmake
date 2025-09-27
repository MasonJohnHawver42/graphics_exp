function(GenerateAssetForgeTarget script_folder input_folder output_folder target_name)
    # Set the rules file path
    set(rules_file "${input_folder}/rules.json")

    # Ensure the script folder exists
    if (NOT EXISTS "${script_folder}/build.py")
        message(FATAL_ERROR "The Python build script was not found in ${script_folder}")
    endif()

    # Find Python3
    find_package(Python3 REQUIRED)

    # Add the custom command to run the Python script
    add_custom_command(
        OUTPUT ${output_folder}/generated_marker  # Dummy file to track the command execution
        COMMAND python3 ${script_folder}/build.py
            --script_folder ${script_folder}
            --input_folder ${input_folder}
            --output_folder ${output_folder}
            --rules_file ${rules_file}
        DEPENDS ${script_folder}/build.py ${rules_file}
        COMMENT "Running asset conversion and linking..."
    )

    # Add a custom target for the Python script
    add_custom_target(${target_name} ALL
        DEPENDS ${output_folder}/generated_marker
    )

    # Ensure that other targets can depend on this one
    message(STATUS "Custom target ${target_name} created. Subdirectories can add dependencies to it.")
endfunction()