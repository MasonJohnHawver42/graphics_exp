#include "debug.h"

#include "data/debug_primitives_generated.h"

#include <cstring>             // for memcpy

gl::debug::IRenderer::~IRenderer() = default;

gl::debug::ModelCacheCreateInfo::ModelCacheCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr)
    : models(mr), primitives(mr), vertices(mr), indices(mr) {

    auto buf = gl::debug::GetDebugPrimitives(data);
    
    // Build model name map
    for (size_t i = 0; i < buf->model_names()->size(); ++i) {
        std::string name(buf->model_names()->Get(i)->c_str());
        models[name] = static_cast<unsigned int>(i);
    }

    // Model offsets
    for (auto model : *buf->model_offsets()) {
        primitives.emplace_back(Model{
            model->vert_offset(),
            model->ebo_offset(),
            model->vert_length(),
            model->ebo_length()
        });
    }

    // Vertices
    for (auto v : *buf->vertices()) {
        Vertex vert;
        std::memcpy(vert.position, v->position()->data(), 3 * sizeof(float));
        std::memcpy(vert.normal, v->normal()->data(), 3 * sizeof(float));
        vertices.push_back(vert);
    }

    // Indices
    for (auto idx : *buf->indices()) {
        indices.push_back(idx);
    }
}

gl::debug::ModelCacheCreateInfo::ModelCacheCreateInfo(const cm::MappedFile& cache_file, std::pmr::memory_resource* mr) :
    ModelCacheCreateInfo(cache_file.data(), cache_file.mappedSize(), mr) {}


std::ostream& operator<<(std::ostream& os, const gl::debug::ModelCacheCreateInfo& info) {
    // === Print Models ===
    os << "\n=== Models ===\n";
    os << std::setw(4)  << "Idx"
        << std::setw(16) << "Name"
        << std::setw(12) << "VertOffset"
        << std::setw(12) << "EboOffset"
        << std::setw(12) << "VertLength"
        << std::setw(12) << "EboLength" << '\n';

    for (const auto& [name, idx] : info.models) {
        const auto& m = info.primitives[idx];
        os << std::setw(4) << idx
            << std::setw(16) << name
            << std::setw(12) << m.vert_offset
            << std::setw(12) << m.ebo_offset
            << std::setw(12) << m.vert_length
            << std::setw(12) << m.ebo_length << '\n';
    }

    // === Print Vertices ===
    os << "\n=== Vertices ===\n";
    os << std::setw(6) << "Index"
        << std::setw(8) << "Px"
        << std::setw(8) << "Py"
        << std::setw(8) << "Pz"
        << std::setw(8) << "Nx"
        << std::setw(8) << "Ny"
        << std::setw(8) << "Nz" << '\n';

    for (size_t i = 0; i < info.vertices.size(); ++i) {
        const auto& v = info.vertices[i];
        os << std::setw(6) << i
            << std::setw(8) << v.position[0]
            << std::setw(8) << v.position[1]
            << std::setw(8) << v.position[2]
            << std::setw(8) << v.normal[0]
            << std::setw(8) << v.normal[1]
            << std::setw(8) << v.normal[2] << '\n';
    }

    // === Print Indices ===
    os << "\n=== Indices (Triangles) ===\n";
    os << std::setw(6) << "Tri"
        << std::setw(6) << "i0"
        << std::setw(6) << "i1"
        << std::setw(6) << "i2" << '\n';

    const auto& indices = info.indices;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        os << std::setw(6) << (i / 3)
            << std::setw(6) << indices[i]
            << std::setw(6) << indices[i + 1]
            << std::setw(6) << indices[i + 2] << '\n';
    }

    return os;
}

gl::debug::ModelCache::ModelCache(const ModelCacheCreateInfo& params, std::pmr::memory_resource* mr)
    : models(mr), primitives(mr), vao(0), vbo(0), ebo(0)
{
    // Copy model name map and offsets
    models = params.models;
    primitives = params.primitives;

    // Flatten all vertices
    std::vector<float> vertex_data;
    for (const auto& v : params.vertices) {
        vertex_data.insert(vertex_data.end(), std::begin(v.position), std::end(v.position));
        vertex_data.insert(vertex_data.end(), std::begin(v.normal), std::end(v.normal));
    }

    // Create VAO, VBO, EBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, params.indices.size() * sizeof(unsigned int), params.indices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers
    constexpr size_t stride = sizeof(float) * 6; // 3 pos + 3 normal
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));

    glBindVertexArray(0);
}

gl::debug::ModelCache::~ModelCache() {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}

gl::debug::NaiveRenderer::NaiveRenderer(const ShaderProgramCreateInfo& shader_param, const ModelCacheCreateInfo& cache_param, std::pmr::memory_resource* mr) :
    program(shader_param, mr), cache(cache_param, mr), draw_queue(mr) 
{
    u_model = program.getUniformLocation("u_model", GL_FLOAT_MAT4);
    u_view = program.getUniformLocation("u_view", GL_FLOAT_MAT4);
    u_color = program.getUniformLocation("u_color", GL_FLOAT_VEC3);
}

void gl::debug::NaiveRenderer::begin(const FrameCommand& cmd) {
    current_frame = cmd;
    draw_queue.clear();
}

void gl::debug::NaiveRenderer::submit(const DrawCommand& cmd) {
    draw_queue.push_back(cmd);
}

void gl::debug::NaiveRenderer::end() {
    program.use();
    glBindVertexArray(cache.vao);

    glUniformMatrix4fv(u_view, 1, GL_FALSE, &current_frame.view[0][0]);

    for (const auto& draw : draw_queue) {
        if (draw.id >= cache.primitives.size()) {
            // std::cerr << "[NaiveRenderer] Invalid model id: " << draw.id << std::endl;
            continue;
        }

        const Model& model = cache.primitives[draw.id];

        glUniformMatrix4fv(u_model, 1, GL_FALSE, &draw.model[0][0]);
        glUniform3fv(u_color, 1, &draw.color[0]);

        void* ebo_offset = reinterpret_cast<void*>(model.ebo_offset * sizeof(unsigned int));
        glDrawElements(GL_TRIANGLES, model.ebo_length, GL_UNSIGNED_INT, ebo_offset);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

unsigned int gl::debug::NaiveRenderer::model_id(std::string&& query) {
    auto it = cache.models.find(query);
    if (it != cache.models.end()) {
        return it->second;
    }
    std::string error_msg = std::string("[NaiveRenderer] model_id(): Could not find ") + query;
    ASSERT(false, error_msg);
}