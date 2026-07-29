function(embed_file INPUT OUTPUT VARIABLE)
    file(READ "${INPUT}" RAW)
    file(APPEND "${OUTPUT}"
"inline constexpr char ${VARIABLE}[] = R\"PICO(
${RAW}
)PICO\";

")
endfunction()
