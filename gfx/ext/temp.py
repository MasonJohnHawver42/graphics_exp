import re

seen = set()

with open("glad/include/glad/glad.h") as f:
    for line in f:
        if line.startswith("#define GL_"):
            tokens = line.split()
            if len(tokens) >= 3:
                name = tokens[1]
                value = tokens[2]

                # Skip if we've already seen this GLenum value
                if value in seen:
                    continue

                seen.add(value)
                print(f'{{ {value}, "{name}" }},')

