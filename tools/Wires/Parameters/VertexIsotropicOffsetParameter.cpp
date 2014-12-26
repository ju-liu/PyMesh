#include "VertexIsotropicOffsetParameter.h"
#include "IsotropicTransforms.h"

VertexIsotropicOffsetParameter::VertexIsotropicOffsetParameter(
        WireNetwork::Ptr wire_network, size_t axis)
: PatternParameter(wire_network), m_axis(axis) {
    assert(m_axis < m_wire_network->get_dim());
}

void VertexIsotropicOffsetParameter::apply(VectorF& results,
        const PatternParameter::Variables& vars) {
    const size_t dim = m_wire_network->get_dim();
    const size_t num_vertices = m_wire_network->get_num_vertices();
    const size_t roi_size = m_roi.size();
    const VectorF center = m_wire_network->center();
    assert(m_axis < dim);
    assert(results.size() == dim * num_vertices);
    assert(roi_size == m_transforms.size());

    if (m_formula != "") evaluate_formula(vars);

    const MatrixFr& vertices = m_wire_network->get_vertices();
    size_t seed_vertex_index = m_roi.minCoeff();
    VectorF seed_vertex = vertices.row(seed_vertex_index);
    VectorF seed_offset = VectorF::Zero(dim);
    seed_offset[m_axis] = (seed_vertex[m_axis] - center[m_axis]) * m_value;

    for (size_t i=0; i<roi_size; i++) {
        size_t v_idx = m_roi[i];
        assert(v_idx < num_vertices);
        const MatrixF& trans = m_transforms[i];
        results.segment(v_idx*dim, dim) += trans * seed_offset;
    }
}

MatrixFr VertexIsotropicOffsetParameter::compute_derivative() const {
    const VectorF center = m_wire_network->center();
    const size_t dim = m_wire_network->get_dim();
    const size_t num_vertices = m_wire_network->get_num_vertices();
    const size_t roi_size = m_roi.size();
    const MatrixFr& vertices = m_wire_network->get_vertices();
    assert(m_axis < dim);
    assert(roi_size == m_transforms.size());

    size_t seed_vertex_index = m_roi.minCoeff();
    VectorF seed_vertex = vertices.row(seed_vertex_index);
    VectorF seed_offset = VectorF::Zero(dim);
    seed_offset[m_axis] = seed_vertex[m_axis] - center[m_axis];

    MatrixFr derivative = MatrixFr::Zero(num_vertices, dim);
    for (size_t i=0; i<roi_size; i++) {
        size_t v_idx = m_roi[i];
        assert(v_idx < num_vertices);
        const MatrixF& trans = m_transforms[i];
        derivative.row(v_idx) = trans * seed_offset;
    }

    return derivative;
}

void VertexIsotropicOffsetParameter::process_roi() {
    const size_t dim = m_wire_network->get_dim();
    MatrixFr vertices = m_wire_network->get_vertices();
    VectorF center = m_wire_network->center();

    m_transforms.clear();
    IsotropicTransforms iso_trans(dim);
    size_t roi_size = m_roi.size();
    size_t seed_vertex_index = m_roi.minCoeff();
    VectorF seed_dir = vertices.row(seed_vertex_index).transpose() - center;

    for (size_t i=0; i<roi_size; i++) {
        VectorF v_dir = vertices.row(m_roi[i]).transpose() - center;
        MatrixF trans = iso_trans.fit(seed_dir, v_dir);
        m_transforms.push_back(trans);
    }
}
