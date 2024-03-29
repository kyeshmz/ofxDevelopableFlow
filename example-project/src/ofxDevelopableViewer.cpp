/*
 
 2018, Oded Stein, Eitan Grinspun and Keenan Crane
 
 This file is part of the code for "Developability of Triangle Meshes".
 
 The code for "Developability of Triangle Meshes" is free software: you can
 redistribute it and/or modify it under the terms of the GNU General Public
 License as published by the Free Software Foundation, either version 2 of the
 License, or (at your option) any later version.
 
 The code for "Developability of Triangle Meshes" is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with the code for "Developability of Triangle Meshes". If not,
 see <https://www.gnu.org/licenses/>.
 
 */



#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>

#include <igl/opengl/gl.h>
#include <igl/quat_to_mat.h>
#include <igl/barycenter.h>
#include <igl/frustum.h>
#include <igl/ortho.h>
#include <igl/edges.h>
//#include <igl/two_axis_valuator_fixed_up.h>
#include <igl/look_at.h>
#include <igl/trackball.h>
#include <igl/project.h>
#include <igl/unproject.h>
#include <igl/snap_to_canonical_view_quat.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>
#include "Viewer.h"

#include "shaders.txt"


    
    //CLASS INITIALIZATION
    
    IGL_INLINE void ofxDevelopableViewer::defaults()
    {
        backgroundColor = Eigen::Vector3f(0.9, 0.9, 0.9);
        wireColor = Eigen::Vector3f(0., 0., 0.);
        defaultColor = Eigen::Vector3d(0., 118./255., 189./255.);
        set_colors(defaultColor);
        defaultColorUsed = true;
        m_linetransp = Eigen::VectorXf::Constant(m_E_mod.rows(), 1.);
        
        pointsOutlineColor = Eigen::Vector3f(0., 0., 0.);
        pointsFillColor = Eigen::Vector3f(255./255., 255./255., 179./255.);
        pointsImportantColor = Eigen::Vector3f(141./255., 211./255., 199./255.);
        pointsSelectedColor = Eigen::Vector3f(225./255., 26./255., 39./255.);
        
        //cameraRotation = Eigen::Quaternion<float>::Identity();
        cameraRotation = Eigen::AngleAxis<float>(-0.1*M_PI, Eigen::Vector3f(1., 0., 0.));
        cameraTranslation = Eigen::Vector3f::Zero();
        
        camera_eye = Eigen::Vector3f(0., 0., 5.);
        camera_center = Eigen::Vector3f(0., 0., 0.);
        camera_up = Eigen::Vector3f(0., 1., 0.);
        
        lightPosition = Eigen::Vector3f(0.0, 0.3, 5.0);
        
        plotColors = {Eigen::Vector3f(31./255., 120./255., 180./255.),
            Eigen::Vector3f(51./255., 160./255., 44./255.),
            Eigen::Vector3f(227./255., 26./255., 28./255.),
            Eigen::Vector3f(255./255., 127./255., 0./255.),
            Eigen::Vector3f(106./255., 61./255., 154./255.),
            Eigen::Vector3f(177./255., 89./255., 40./255.),
            Eigen::Vector3f(166./255., 206./255., 227./255.),
            Eigen::Vector3f(178./255., 223./255., 138./255.),
            Eigen::Vector3f(251./255., 154./255., 153./255.),
            Eigen::Vector3f(253./255., 191./255., 111./255.),
            Eigen::Vector3f(202./255., 178./255., 214./255.),
            Eigen::Vector3f(255./255., 255./255., 153./255.)};
        plotBGColor = Eigen::Vector3f(0.97, 0.97, 0.97);
    }
    
    IGL_INLINE ofxDevelopableViewer::ofxDevelopableViewer()
    {
        defaults();
    }
    
    IGL_INLINE ofxDevelopableViewer::ofxDevelopableViewer(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F)
    {
        orig_set_mesh(V, F);
        defaults();
    }
    
    IGL_INLINE ofxDevelopableViewer::ofxDevelopableViewer(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, const Eigen::MatrixXi& E)
    {
        orig_set_mesh(V, F, E);
        defaults();
    }
    
    
    
    //GETTERS & SETTERS
    
IGL_INLINE void ofxDevelopableViewer::orig_set_mesh(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F)
{
    Eigen::MatrixXi E;
    igl::edges(F, E);
    orig_set_mesh(V, F, E);
}

IGL_INLINE void ofxDevelopableViewer::orig_set_mesh(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, const Eigen::MatrixXi& E)
{
    assert(V.cols()==3 && "Every vertex must have 3 coordinates.\n");
    m_V = V.cast<float>();
    m_V_mod = m_V;
    
    //Center and scale
    float scale = 1.;
    Eigen::Vector3f transl;
    if(V.rows() != 0) {
        Eigen::MatrixXd barycenters;
        igl::barycenter(V, F, barycenters);
        Eigen::Vector3d minPoint = barycenters.colwise().minCoeff();
        Eigen::Vector3d maxPoint = barycenters.colwise().maxCoeff();
        Eigen::Vector3d center = 0.5*(minPoint + maxPoint);
        transl = -center.cast<float>();
        scale = 2. / (maxPoint-minPoint).array().abs().maxCoeff();
    }
    initialShift = Eigen::Affine3f::Identity();
    initialShift.scale(scale);
    initialShift.translate(transl);
    
    assert(F.cols()==3 && "Every face must have 3 vertices.\n");
    m_F = F.cast<GLuint>();
    m_F_mod = m_F;
    
    igl::per_face_normals(m_V, m_F_mod, m_facedVN_mod);
    igl::per_vertex_normals(m_V, m_F_mod, m_facedVN_mod, m_VN);
    m_VN_mod = m_VN;
    
    assert(E.cols()==2 && "Every edge must have 2 vertices.\n");
    m_E = E.cast<GLuint>();
    m_E_mod = m_E;
    
    meshSet = true;
    
    if(launched) {
        update_mesh_data(true);
        if(defaultColorUsed) {
            set_colors(defaultColor);
            defaultColorUsed = true;
        }
        linetranspEnabled = false;
    }
}


IGL_INLINE void ofxDevelopableViewer::set_colors(const Eigen::Vector3d& color)
{
    Eigen::MatrixXd colors(faceBasedColoring ? m_F_mod.rows() : m_V_mod.rows(), 3);
    for(int i=0; i<colors.rows(); ++i)
        colors.row(i) = color;
    
    set_colors(colors);
}

IGL_INLINE void ofxDevelopableViewer::set_colors(const Eigen::MatrixXd& color)
{
    set_colors(0.4*color, 0.3 + 0.2*(color.array()-0.3), color);
}

IGL_INLINE void ofxDevelopableViewer::set_colors(const Eigen::MatrixXd& color_amb, const Eigen::MatrixXd& color_spec, const Eigen::MatrixXd& color_dif)
{
    assert(color_amb.cols() == 3 && color_spec.cols() == 3 && color_dif.cols() == 3 && "Color matrices must have 3 cols.\n");
    assert(color_amb.rows() == color_spec.rows() && color_spec.rows() == color_dif.rows() && "Color matrices must all have the same number of rows.\n");
    assert( (color_amb.rows() == m_V_mod.rows() || color_amb.rows() == m_F_mod.rows() || color_amb.rows() == 3*m_F_mod.rows()) && "Color matrices must either have the same number of rows as vertices, or as faces in the mesh.\n");
    
    m_amb = color_amb.cast<float>();
    m_spec = color_spec.cast<float>();
    m_dif = color_dif.cast<float>();
    
    if(color_amb.rows() == m_F_mod.rows() || color_amb.rows() == 3*m_F_mod.rows())
        faceBasedColoring = true;
    else if(color_amb.rows() == m_V.rows())
        faceBasedColoring = false;
    
    if(colorsBuffersGenerated)
        update_colors();
    
    defaultColorUsed = false;
}

IGL_INLINE void ofxDevelopableViewer::set_linetransparency(const Eigen::VectorXd& transp)
{
    m_linetransp = transp.cast<float>();
    
    bool linetranspWasEnabled = linetranspEnabled;
    linetranspEnabled = true;
    if(launched) {
        if(!linetranspWasEnabled)
            update_mesh_data();
        if(linetranspBufferGenerated)
            update_linetransp();
    }
}


IGL_INLINE void ofxDevelopableViewer::set_face_based_coloring(const bool& colorMode)
{
    faceBasedColoring = colorMode;
    
    if(colorsBuffersGenerated)
        update_colors();
}

IGL_INLINE bool ofxDevelopableViewer::get_face_based_coloring()
{
    return faceBasedColoring;
}

IGL_INLINE const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>& ofxDevelopableViewer::get_V() const
{
    return m_V_mod;
}

IGL_INLINE void ofxDevelopableViewer::orig_modify_V(const Eigen::MatrixXd& V)
{
    m_V_mod = V.cast<float>();
    igl::per_face_normals(m_V_mod, m_F_mod, m_facedVN_mod);
    igl::per_vertex_normals(m_V_mod, m_F_mod, m_facedVN_mod, m_VN_mod);
    update_mesh_data(false);
    
    if(defaultColorUsed) {
        set_colors(defaultColor);
        defaultColorUsed = true;
    }
}


IGL_INLINE void ofxDevelopableViewer::modify_F(const Eigen::MatrixXi& F, Eigen::MatrixXi E)
{
    m_F_mod = F.cast<GLuint>();
    igl::per_face_normals(m_V_mod, m_F_mod, m_facedVN_mod);
    igl::per_vertex_normals(m_V_mod, m_F_mod, m_facedVN_mod, m_VN_mod);
    
    if(E.rows()==0)
        igl::edges(F, E);
    m_E_mod = E.cast<GLuint>();
    
    update_mesh_data(true);
    linetranspEnabled = false;
}


IGL_INLINE int ofxDevelopableViewer::get_width() const
{
    return windowWidth;
}

IGL_INLINE int ofxDevelopableViewer::get_height() const
{
    return windowHeight;
}

IGL_INLINE void ofxDevelopableViewer::set_width(const int& w)
{
    assert(w>0 && "Width must be positive.\n");
    windowWidth = w;
}

IGL_INLINE void ofxDevelopableViewer::set_height(const int& h)
{
    assert(h>0 && "Height must be positive.\n");
    windowHeight = h;
}

IGL_INLINE void ofxDevelopableViewer::set_window_size(const int& w, const int& h)
{
    set_width(w);
    set_height(h);
}

IGL_INLINE ofxDevelopableViewer::ProjectionType ofxDevelopableViewer::get_projection_type() const
{
    return projectionType;
}

IGL_INLINE void ofxDevelopableViewer::set_projection_type(const ProjectionType& type)
{
    projectionType = type;
}

IGL_INLINE ofxDevelopableViewer::DisplayMode ofxDevelopableViewer::get_display_mode() const
{
    return displayMode;
}

IGL_INLINE void ofxDevelopableViewer::set_display_mode(const DisplayMode& mode)
{
    displayMode = mode;
}


IGL_INLINE bool ofxDevelopableViewer::get_dragging_enabled() const
{
    return draggingEnabled;
}

IGL_INLINE void ofxDevelopableViewer::set_dragging_enabled(const bool& mode)
{
    draggingEnabled = mode;
}


IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_background_color() const
{
    return backgroundColor;
}

IGL_INLINE void ofxDevelopableViewer::set_background_color(const Eigen::Vector3d& color)
{
    backgroundColor = color.cast<float>();
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_wire_color() const
{
    return wireColor;
}

IGL_INLINE void ofxDevelopableViewer::set_wire_color(const Eigen::Vector3d& color)
{
    wireColor = color.cast<float>();
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_points_outline_color() const
{
    return pointsOutlineColor;
}

IGL_INLINE void ofxDevelopableViewer::set_points_outline_color(const Eigen::Vector3d& color)
{
    pointsOutlineColor = color.cast<float>();
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_points_fill_color() const
{
    return pointsFillColor;
}

IGL_INLINE void ofxDevelopableViewer::set_points_fill_color(const Eigen::Vector3d& color)
{
    pointsFillColor = color.cast<float>();
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_points_important_color() const
{
    return pointsImportantColor;
}

IGL_INLINE void ofxDevelopableViewer::set_points_important_color(const Eigen::Vector3d& color)
{
    pointsImportantColor = color.cast<float>();
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_points_selected_color() const
{
    return pointsSelectedColor;
}

IGL_INLINE void ofxDevelopableViewer::set_points_selected_color(const Eigen::Vector3d& color)
{
    pointsSelectedColor = color.cast<float>();
}


IGL_INLINE void ofxDevelopableViewer::set_point_size(const float& size)
{
    set_point_size(size, size, 4./3.*size);
}

IGL_INLINE void ofxDevelopableViewer::set_point_size(const float& size, const float& importantSize, const float& selectedSize)
{
    pointSize = size;
    importantPointSize = importantSize;
    selectedPointSize = selectedSize;
}


IGL_INLINE float ofxDevelopableViewer::get_camera_zoom() const
{
    return cameraZoom;
}

IGL_INLINE void ofxDevelopableViewer::set_camera_zoom(const float& zoom)
{
    cameraZoom = zoom;
}

IGL_INLINE Eigen::Quaternion<float> ofxDevelopableViewer::get_camera_rotation() const
{
    return cameraRotation;
}

IGL_INLINE void ofxDevelopableViewer::set_camera_rotation(const Eigen::Quaternion<float>& rotation)
{
    cameraRotation = rotation;
}

IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_camera_translation() const
{
    return cameraTranslation;
}

IGL_INLINE void ofxDevelopableViewer::set_camera_translation(const Eigen::Vector3f& translation)
{
    cameraTranslation = translation;
}


IGL_INLINE bool ofxDevelopableViewer::get_rotation_enabled() const
{
    return rotationEnabled;
}

IGL_INLINE void ofxDevelopableViewer::set_rotation_enabled(const bool& enabled)
{
    rotationEnabled = enabled;
}


IGL_INLINE Eigen::Vector3f ofxDevelopableViewer::get_light_position() const
{
    return lightPosition;
}

IGL_INLINE void ofxDevelopableViewer::set_light_position(const Eigen::Vector3f& position)
{
    lightPosition = position;
    if(colorsBuffersGenerated)
        update_lighting();
}

IGL_INLINE float ofxDevelopableViewer::get_lighting_factor() const
{
    return lightingFactor;
}

IGL_INLINE void ofxDevelopableViewer::set_lighting_factor(const float& factor)
{
    lightingFactor = factor;
    if(colorsBuffersGenerated)
        update_lighting();
}

IGL_INLINE float ofxDevelopableViewer::get_shininess() const
{
    return shininess;
}

IGL_INLINE void ofxDevelopableViewer::set_shininess(const float& shiny)
{
    shininess = shiny;
    if(colorsBuffersGenerated)
        update_lighting();
}

IGL_INLINE bool ofxDevelopableViewer::get_cel_shading() const
{
    return celShading;
}

IGL_INLINE void ofxDevelopableViewer::set_cel_shading(const bool& cel)
{
    celShading = cel;
    if(colorsBuffersGenerated)
        update_lighting();
}


IGL_INLINE void ofxDevelopableViewer::mark_points(const Eigen::VectorXi& points)
{
    markedPoints = points;
    pointIsImportant = Eigen::VectorXi::Constant(points.size(), false);
    pointIsSelected = Eigen::VectorXi::Constant(points.size(), false);
    markedPointsTranslations = Eigen::MatrixXd::Constant(points.size(), 3, 0.);
}

IGL_INLINE const Eigen::VectorXi& ofxDevelopableViewer::get_marked_points() const
{
    return markedPoints;
}

IGL_INLINE void ofxDevelopableViewer::set_important_points(const Eigen::VectorXi& points)
{
    pointIsImportant = points;
}

IGL_INLINE const Eigen::VectorXi& ofxDevelopableViewer::get_important_points() const
{
    return pointIsImportant;
}

IGL_INLINE const Eigen::VectorXi& ofxDevelopableViewer::get_selected_points() const
{
    return pointIsSelected;
}

IGL_INLINE const Eigen::MatrixXd& ofxDevelopableViewer::get_marked_points_translations() const
{
    return markedPointsTranslations;
}


IGL_INLINE const Eigen::Matrix4f& ofxDevelopableViewer::get_model() const
{
    return model;
}

IGL_INLINE const Eigen::Matrix4f& ofxDevelopableViewer::get_view() const
{
    return view;
}

IGL_INLINE const Eigen::Matrix4f& ofxDevelopableViewer::get_proj() const
{
    return proj;
}


IGL_INLINE void ofxDevelopableViewer::stop_dragging()
{
    currentlyDragging = false;
}

IGL_INLINE bool ofxDevelopableViewer::get_currently_dragging() const
{
    return currentlyDragging;
}


IGL_INLINE bool ofxDevelopableViewer::showLines() const
{
    return displayMode == Wireframe || displayMode == FillWireframe || displayMode == TextureWireframe;
}

IGL_INLINE bool ofxDevelopableViewer::showFill() const
{
    return displayMode == Fill || displayMode == FillWireframe;
}

IGL_INLINE bool ofxDevelopableViewer::showTexture() const
{
    return displayMode == Texture || displayMode == TextureWireframe;
}


IGL_INLINE void ofxDevelopableViewer::set_plottitles(const std::vector<std::string>& titles)
{
    plotTitles = titles;
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE void ofxDevelopableViewer::set_plotlines(const std::vector<Eigen::VectorXd>& lines)
{
    plotLines = lines;
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE void ofxDevelopableViewer::set_plots(const std::vector<std::string>& titles, const std::vector<Eigen::VectorXd>& lines)
{
    plotTitles = titles;
    plotLines = lines;
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE void ofxDevelopableViewer::set_plotcolors(const std::vector<Eigen::Vector3d>& colors)
{
    plotColors = std::vector<Eigen::Vector3f>(colors.size());
    for(int i=0; i<plotColors.size(); ++i) {
        plotColors[i] = colors[i].cast<float>();
    }
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE void ofxDevelopableViewer::set_plotting_enabled(const bool& mode)
{
    plottingEnabled = mode;
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE bool ofxDevelopableViewer::get_plotting_enabled()
{
    return plottingEnabled;
}

IGL_INLINE void ofxDevelopableViewer::set_plotting_logplot(const bool& mode)
{
    plottingLogPlot = mode;
    if(plotBuffersGenerated)
        update_plots();
}

IGL_INLINE bool ofxDevelopableViewer::get_plotting_logplot()
{
    return plottingLogPlot;
}

    
    //GRAPHICS
    
/*
 
 2018, Oded Stein, Eitan Grinspun and Keenan Crane
 
 This file is part of the code for "Developability of Triangle Meshes".
 
 The code for "Developability of Triangle Meshes" is free software: you can
 redistribute it and/or modify it under the terms of the GNU General Public
 License as published by the Free Software Foundation, either version 2 of the
 License, or (at your option) any later version.
 
 The code for "Developability of Triangle Meshes" is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with the code for "Developability of Triangle Meshes". If not,
 see <https://www.gnu.org/licenses/>.
 
 */


//All GLFW and OpenGL-related functions in the ofxDevelopableViewer

/*static bool CTRL_pressed = false;
 static bool ALT_pressed = false;
 static bool SHIFT_pressed = false;
 static bool LMB_pressed = false;
 static bool RMB_pressed = false;
 static bool MMB_pressed = false;
 static bool MOD_happened = false;
 
 static double mouse_x, mouse_y, down_mouse_x, down_mouse_y, down_mouse_z=0.;
 static Eigen::Quaternion<float> down_rot;
 static Eigen::Vector3f down_translation;
 static Eigen::MatrixXd marked_down_translation;*/


#include <igl/igl_inline.h>
IGL_INLINE void ofxDevelopableViewer::update_projection_matrix()
{
    proj = Eigen::Matrix4f::Identity();
    const float ratio = ((float) windowWidth)/((float) windowHeight);
    if (projectionType == Orthographic) {
        float length = (camera_eye - camera_center).norm();
        float h = tan(cameraAngle)*length;
        float w = h*ratio;
        igl::ortho(-w, w, -h, h, near, far, proj);
    } else if(projectionType == Frustum) {
        float fH = tan(cameraAngle) * near;
        float fW = fH*ratio;
        igl::frustum(-fW, fW, -fH, fH, near, far, proj);
    }
    
    // select program and attach uniforms
    glUseProgram(fill_prog_id);
    GLint proj_loc = glGetUniformLocation(fill_prog_id,"proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.data());
    glUseProgram(texture_prog_id);
    proj_loc = glGetUniformLocation(texture_prog_id,"proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.data());
    glUseProgram(lines_prog_id);
    proj_loc = glGetUniformLocation(lines_prog_id,"proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.data());
    glUseProgram(points_prog_id);
    proj_loc = glGetUniformLocation(points_prog_id,"proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.data());
}

IGL_INLINE void ofxDevelopableViewer::update_view_matrix()
{
    Eigen::Matrix4f view;
    igl::look_at(camera_eye, camera_center, camera_up, view);
    
    // select program and attach uniforms
    glUseProgram(fill_prog_id);
    GLint view_loc = glGetUniformLocation(fill_prog_id,"view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    glUseProgram(texture_prog_id);
    view_loc = glGetUniformLocation(texture_prog_id,"view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    glUseProgram(lines_prog_id);
    view_loc = glGetUniformLocation(lines_prog_id,"view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    glUseProgram(points_prog_id);
    view_loc = glGetUniformLocation(points_prog_id,"view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    
    this->view = view.matrix();
}

IGL_INLINE void ofxDevelopableViewer::update_model_matrix()
{
    //Camera transform
    Eigen::Affine3f model;
    model.setIdentity();
    model.scale(cameraZoom);
    model.rotate(cameraRotation);
    model = model*initialShift;
    model.translate(cameraTranslation);
    
    // select program and attach uniforms
    glUseProgram(fill_prog_id);
    GLint model_loc = glGetUniformLocation(fill_prog_id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.matrix().data());
    glUseProgram(texture_prog_id);
    model_loc = glGetUniformLocation(texture_prog_id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.matrix().data());
    glUseProgram(lines_prog_id);
    model_loc = glGetUniformLocation(lines_prog_id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.matrix().data());
    glUseProgram(points_prog_id);
    model_loc = glGetUniformLocation(points_prog_id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.matrix().data());
    
    this->model = model.matrix();
}

IGL_INLINE void ofxDevelopableViewer::update_lighting()
{
    glUseProgram(fill_prog_id);
    GLint lightpos_loc = glGetUniformLocation(fill_prog_id, "light_position");
    GLint lightfact_loc = glGetUniformLocation(fill_prog_id, "lighting_factor");
    GLint shininess_loc = glGetUniformLocation(fill_prog_id, "shininess");
    GLint celshading_loc = glGetUniformLocation(fill_prog_id, "cel_shading");
    glUniform3fv(lightpos_loc, 1, lightPosition.data());
    glUniform1f(lightfact_loc, lightingFactor);
    glUniform1f(shininess_loc, shininess);
    glUniform1i(celshading_loc, celShading);
    
    glUseProgram(texture_prog_id);
    lightpos_loc = glGetUniformLocation(texture_prog_id, "light_position");
    lightfact_loc = glGetUniformLocation(texture_prog_id, "lighting_factor");
    shininess_loc = glGetUniformLocation(texture_prog_id, "shininess");
    celshading_loc = glGetUniformLocation(texture_prog_id, "cel_shading");
    glUniform3fv(lightpos_loc, 1, lightPosition.data());
    glUniform1f(lightfact_loc, lightingFactor);
    glUniform1f(shininess_loc, shininess);
    glUniform1i(celshading_loc, celShading);
    
    
}

IGL_INLINE void ofxDevelopableViewer::update_colors()
{
    //Wire colors
    glUseProgram(lines_prog_id);
    GLint wirecolor_loc = glGetUniformLocation(lines_prog_id,"wirecolor");
    glUniform3fv(wirecolor_loc, 1, wireColor.data());
    
    //Point colors
    glUseProgram(points_prog_id);
    GLint outlinecolor_loc = glGetUniformLocation(points_prog_id, "outlinecolor");
    glUniform3fv(outlinecolor_loc, 1, pointsOutlineColor.data());
    GLint pointcolor_loc = glGetUniformLocation(points_prog_id, "pointcolor");
    glUniform3fv(pointcolor_loc, 1, pointsFillColor.data());
    GLint importantcolor_loc = glGetUniformLocation(points_prog_id, "importantcolor");
    glUniform3fv(importantcolor_loc, 1, pointsImportantColor.data());
    GLint selectedcolor_loc = glGetUniformLocation(points_prog_id, "selectedcolor");
    glUniform3fv(selectedcolor_loc, 1, pointsSelectedColor.data());
    
    //Point sizes
    glUseProgram(points_prog_id);
    GLint pointsize_loc = glGetUniformLocation(points_prog_id, "pointsize");
    glUniform1f(pointsize_loc, pointSize);
    GLint importantpointsize_loc = glGetUniformLocation(points_prog_id, "importantpointsize");
    glUniform1f(importantpointsize_loc, importantPointSize);
    GLint selectedpointsize_loc = glGetUniformLocation(points_prog_id, "selectedpointsize");
    glUniform1f(selectedpointsize_loc, selectedPointSize);
    
    //Fill colors
    glUseProgram(fill_prog_id);
    
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> facedAmb, facedSpec, facedDif;
    if(faceBasedColoring) {
        if(m_amb.rows() == m_V_mod.rows()) {
            facedAmb.resize(m_F_mod.rows()*3, 3);
            facedSpec.resize(m_F_mod.rows()*3, 3);
            facedDif.resize(m_F_mod.rows()*3, 3);
            for (unsigned i=0; i<m_F_mod.rows(); ++i) {
                for (unsigned j=0;j<3;++j) {
                    facedAmb.row(i*3+j) = m_amb.row(m_F_mod(i,j));
                    facedSpec.row(i*3+j) = m_spec.row(m_F_mod(i,j));
                    facedDif.row(i*3+j) = m_dif.row(m_F_mod(i,j));
                }
            }
        } else if(m_amb.rows() == m_F_mod.rows()) {
            facedAmb.resize(m_F_mod.rows()*3, 3);
            facedSpec.resize(m_F_mod.rows()*3, 3);
            facedDif.resize(m_F_mod.rows()*3, 3);
            for (unsigned i=0; i<m_F_mod.rows(); ++i) {
                for (unsigned j=0;j<3;++j) {
                    facedAmb.row(i*3+j) = m_amb.row(i);
                    facedSpec.row(i*3+j) = m_spec.row(i);
                    facedDif.row(i*3+j) = m_dif.row(i);
                }
            }
        } else if(m_amb.rows() == 3*m_F_mod.rows()) {
            facedAmb = m_amb;
            facedSpec = m_spec;
            facedDif = m_dif;
        }
    }
    
    glBindVertexArray(VAO);
    
    if(!colorsBuffersGenerated) {
        colorsBuffersGenerated = true;
        glGenBuffers(1, &ambBO);
        glGenBuffers(1, &specBO);
        glGenBuffers(1, &difBO);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, ambBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FBO);
    if(!faceBasedColoring)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_amb.size(), m_amb.data(), GL_STATIC_DRAW);
    else
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*facedAmb.size(), facedAmb.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, specBO);
    if(!faceBasedColoring)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_spec.size(), m_spec.data(), GL_STATIC_DRAW);
    else
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*facedSpec.size(), facedSpec.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, difBO);
    if(!faceBasedColoring)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_dif.size(), m_dif.data(), GL_STATIC_DRAW);
    else
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*facedDif.size(), facedDif.data(), GL_STATIC_DRAW);
    
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    update_linetransp();
}

IGL_INLINE void ofxDevelopableViewer::update_linetransp()
{
    glBindVertexArray(VAO);
    
    if(!linetranspBufferGenerated) {
        linetranspBufferGenerated = true;
        glGenBuffers(1, &linetranspBO);
    }
    
    Eigen::VectorXf col;
    if(linetranspEnabled || faceBasedColoring) {
        col = Eigen::VectorXf(2*m_E_mod.rows());
        for(int i=0; i<m_linetransp.size(); ++i) {
            col(2*i) = m_linetransp(i);
            col(2*i+1) = m_linetransp(i);
        }
    } else {
        col = Eigen::VectorXf::Constant(m_V_mod.rows(), 1.);
    }
    
    //Line transparencies
    glBindBuffer(GL_ARRAY_BUFFER, linetranspBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*col.size(), col.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


IGL_INLINE void ofxDevelopableViewer::update_points()
{
    glBindVertexArray(VAO);
    
    if(!pointsBuffersGenerated) {
        pointsBuffersGenerated = true;
        glGenBuffers(1, &PBO);
        glGenBuffers(1, &importantPBO);
        glGenBuffers(1, &selectedPBO);
    }
    
    pointsCoords.resize(markedPoints.size(), 3);
    for(int i=0; i<markedPoints.size(); ++i)
        pointsCoords.row(i) = m_V_mod.row(markedPoints(i));
    
    glBindBuffer(GL_ARRAY_BUFFER, PBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*pointsCoords.size(), pointsCoords.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, importantPBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*pointIsImportant.size(), pointIsImportant.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, selectedPBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*pointIsSelected.size(), pointIsSelected.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

IGL_INLINE void ofxDevelopableViewer::update_plots()
{
    glBindVertexArray(VAO);
    
    if(!plotBuffersGenerated) {
        plotBuffersGenerated = true;
        glGenBuffers(1, &plotLBO);
        glGenBuffers(1, &plotBBO);
    }
    
    double h = 2*0.2, w = 2*0.9;
    
    //Update background
    double botleftCornerX = 0.1-1.;
    double botleftCornerY = 0.1-1.;
    Eigen::VectorXf bg(6*2);
    bg << botleftCornerX, botleftCornerY,
    botleftCornerX+w, botleftCornerY,
    botleftCornerX, botleftCornerY+h,
    botleftCornerX+w, botleftCornerY,
    botleftCornerX+w, botleftCornerY+h,
    botleftCornerX, botleftCornerY+h;
    glBindBuffer(GL_ARRAY_BUFFER, plotBBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*bg.size(), bg.data(), GL_DYNAMIC_DRAW);
    
    //Update lines
    glBindBuffer(GL_ARRAY_BUFFER, plotLBO);
    int bufSize = 0;
    for(int i=0; i<plotLines.size(); ++i) {
        bufSize += 2*plotLines[i].size();
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*bufSize, NULL, GL_DYNAMIC_DRAW);
    int offset = 0;
    for(int i=0; i<plotLines.size(); ++i) {
        if(plotLines[i].size()==0)
            continue;
        
        const Eigen::VectorXd& data = plotLines[i];
        double min = data.minCoeff();
        double max = data.maxCoeff();
        if(plottingLogPlot) {
            min -= 0.1;
        }
        double scale;
        if(plottingLogPlot) {
            scale = 1./0.9*(log(max-min)-log(0.1));
        } else {
            scale = 1./0.9*(max-min);
        }
        Eigen::Matrix<float, Eigen::Dynamic, 2, Eigen::RowMajor> linepos(data.size(), 2);
        for(int j=0; j<data.size(); ++j) {
            double progr = 0.96*((double) j)/((double) data.size()-1);
            
            if(plottingLogPlot) {
                linepos.row(j) << w*0.02+botleftCornerX + progr*w, h*0.05+botleftCornerY + h/scale*(log(data(j) - min) - log(0.1));
            } else {
                linepos.row(j) << w*0.02+botleftCornerX + progr*w, h*0.05+botleftCornerY + h/scale*(data(j) - min);
            }
        }
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*offset, sizeof(float)*linepos.size(), linepos.data());
        offset += linepos.size();
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


IGL_INLINE void ofxDevelopableViewer::update_mesh_data(const bool& updateFacesEdges)
{
    
    if(!meshBuffersGenerated) {
        meshBuffersGenerated = true;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &edgebasedVBO);
        glGenBuffers(1, &NBO);
        glGenBuffers(1, &FBO);
        glGenBuffers(1, &EBO);
    } else {
        glBindVertexArray(VAO);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    if(faceBasedColoring) {
        Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> reV(3*m_F_mod.rows(), 3);
        for(int i=0; i<m_F_mod.rows(); ++i)
            for(int j=0; j<3; ++j)
                reV.row(3*i+j) = m_V_mod.row(m_F_mod(i,j));
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*reV.size(), reV.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_V_mod.size(), m_V_mod.data(), GL_DYNAMIC_DRAW);
    }
    
    if(linetranspEnabled || faceBasedColoring) {
        glBindBuffer(GL_ARRAY_BUFFER, edgebasedVBO);
        Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> reV(2*m_E_mod.rows(), 3);
        for(int i=0; i<m_E_mod.rows(); ++i)
            for(int j=0; j<2; ++j)
                reV.row(2*i+j) = m_V_mod.row(m_E_mod(i,j));
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*reV.size(), reV.data(), GL_DYNAMIC_DRAW);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    if(faceBasedColoring) {
        Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> reVN(3*m_F_mod.rows(), 3);
        for(int i=0; i<m_F_mod.rows(); ++i)
            for(int j=0; j<3; ++j)
                //reVN.row(3*i+j) = m_VN_mod.row(m_F_mod(i,j));
                reVN.row(3*i+j) = m_facedVN_mod.row(i);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*reVN.size(), reVN.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_VN_mod.size(), m_VN_mod.data(), GL_DYNAMIC_DRAW);
    }
    
    if(updateFacesEdges) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FBO);
        if(faceBasedColoring) {
            Eigen::Matrix<GLuint, Eigen::Dynamic, 3, Eigen::RowMajor> reF(m_F_mod.rows(), 3);
            for(int i=0; i<m_F_mod.rows(); ++i)
                reF.row(i) << 3*i, 3*i+1, 3*i+2;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*reF.size(), reF.data(), GL_DYNAMIC_DRAW);
        } else {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*m_F_mod.size(), m_F_mod.data(), GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        if(linetranspEnabled || faceBasedColoring) {
            Eigen::Matrix<GLuint, Eigen::Dynamic, 2, Eigen::RowMajor> reE(m_E_mod.rows(), 2);
            for(int i=0; i<m_E_mod.rows(); ++i)
                reE.row(i) << 2*i, 2*i+1;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*reE.size(), reE.data(), GL_DYNAMIC_DRAW);
        } else {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*m_E_mod.size(), m_E_mod.data(), GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    update_points();
}


IGL_INLINE GLuint ofxDevelopableViewer::start_program(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader)
{
    const auto & compile_shader = [](const GLint type,const char * str) -> GLuint {
        GLuint id = glCreateShader(type);
        if(id==0)
            std::cout << "Error generating shader." << std::endl;
        glShaderSource(id,1,&str,NULL);
        glCompileShader(id);
        return id;
    };
    
    GLuint vertexShaderId = compile_shader(GL_VERTEX_SHADER, vertexShader.c_str());
    GLuint fragmentShaderId = compile_shader(GL_FRAGMENT_SHADER, fragmentShader.c_str());
    GLuint geometryShaderId;
    if(geometryShader.size() > 0)
        compile_shader(GL_GEOMETRY_SHADER, geometryShader.c_str());
    
    GLuint prog_id = glCreateProgram();
    if(prog_id==0)
        std::cout << "Error generating shader." << std::endl;
    glAttachShader(prog_id, vertexShaderId);
    glAttachShader(prog_id, fragmentShaderId);
    if(geometryShader.size() > 0)
        glAttachShader(prog_id, geometryShaderId);
    glLinkProgram(prog_id);
    
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);
    if(geometryShader.size() > 0)
        glDeleteShader(geometryShaderId);
    GLint status;
    glGetProgramiv(prog_id, GL_LINK_STATUS, &status);
    
    if(!status) {
        std::cout << "Error linking shader. " << std::endl;
    }
    
    return prog_id;
}


IGL_INLINE void ofxDevelopableViewer::drawing()
{
    // clear screen and set viewport
    glClearColor(backgroundColor(0), backgroundColor(1), backgroundColor(2), 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,windowWidth,windowHeight);
    
    glBindVertexArray(VAO);
    
    // Draw mesh
    glEnable(GL_DEPTH_TEST);
    if(showFill()) {
        glUseProgram(fill_prog_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        GLint position_id_fill = glGetAttribLocation(fill_prog_id, "position");
        glVertexAttribPointer(position_id_fill, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(position_id_fill);
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        GLint normal_id_fill = glGetAttribLocation(fill_prog_id, "normal");
        glVertexAttribPointer(normal_id_fill, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0 );
        glEnableVertexAttribArray(normal_id_fill);
        glBindBuffer(GL_ARRAY_BUFFER, ambBO);
        GLint amb_id_fill = glGetAttribLocation(fill_prog_id, "amb");
        glVertexAttribPointer(amb_id_fill, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0 );
        glEnableVertexAttribArray(amb_id_fill);
        glBindBuffer(GL_ARRAY_BUFFER, specBO);
        GLint spec_id_fill = glGetAttribLocation(fill_prog_id, "spec");
        glVertexAttribPointer(spec_id_fill, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0 );
        glEnableVertexAttribArray(spec_id_fill);
        glBindBuffer(GL_ARRAY_BUFFER, difBO);
        GLint dif_id_fill = glGetAttribLocation(fill_prog_id, "dif");
        glVertexAttribPointer(dif_id_fill, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0 );
        glEnableVertexAttribArray(dif_id_fill);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FBO);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, 1.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        glDrawElements(GL_TRIANGLES, m_F_mod.size(), GL_UNSIGNED_INT, 0);
        
        GLenum error = glGetError();
//        if(error != GL_NO_ERROR && error != GL_INVALID_VALUE)
//            std::cout << "There was an OpenGL error while drawing faces. The error is: " << gluErrorString(error) << "." << std::endl;
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        
        glDisableVertexAttribArray(position_id_fill);
        glDisableVertexAttribArray(normal_id_fill);
        glDisableVertexAttribArray(amb_id_fill);
        glDisableVertexAttribArray(spec_id_fill);
        glDisableVertexAttribArray(dif_id_fill);
    }
    if(showLines()) {
        glUseProgram(lines_prog_id);
        if(linetranspEnabled || faceBasedColoring)
            glBindBuffer(GL_ARRAY_BUFFER, edgebasedVBO);
        else
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
        GLint position_id_lines = glGetAttribLocation(lines_prog_id, "position");
        glVertexAttribPointer(position_id_lines, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(position_id_lines);
        glBindBuffer(GL_ARRAY_BUFFER, linetranspBO);
        GLint linetransp_id_lines = glGetAttribLocation(lines_prog_id, "linetransparency");
        glVertexAttribPointer(linetransp_id_lines, 1, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0 );
        glEnableVertexAttribArray(linetransp_id_lines);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glLineWidth(1.);
        
        if(linetranspEnabled){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_LINE_SMOOTH);
        
        glDrawElements(GL_LINES, m_E_mod.size(), GL_UNSIGNED_INT, 0);
        GLenum error = glGetError();
//        if(error != GL_NO_ERROR)
//            std::cout << "There was an OpenGL error while drawing lines. The error is: " << gluErrorString(error) << "." << std::endl;
        
        if(linetranspEnabled)
            glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
        
        glDisableVertexAttribArray(position_id_lines);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    
    // Draw points
    glUseProgram(points_prog_id);
    glBindBuffer(GL_ARRAY_BUFFER, PBO);
    GLint point_position_id = glGetAttribLocation(points_prog_id, "point_position");
    glVertexAttribPointer(point_position_id, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(point_position_id);
    glBindBuffer(GL_ARRAY_BUFFER, importantPBO);
    GLint important_id = glGetAttribLocation(points_prog_id, "isImportant");
    glVertexAttribIPointer(important_id, 1, GL_INT, 0, (GLvoid*)0);
    glEnableVertexAttribArray(important_id);
    glBindBuffer(GL_ARRAY_BUFFER, selectedPBO);
    GLint selected_id = glGetAttribLocation(points_prog_id, "isSelected");
    glVertexAttribIPointer(selected_id, 1, GL_INT, 0, (GLvoid*)0);
    glEnableVertexAttribArray(selected_id);
    
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawArrays(GL_POINTS, 0, markedPoints.size());
    GLenum error = glGetError();
//    if(error != GL_NO_ERROR /*&& error != GL_INVALID_VALUE*/)
//        std::cout << "There was an OpenGL error while drawing points. The error is: " << gluErrorString(error) << "." << std::endl;
    
    glDisable(GL_BLEND);
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    
    glDisableVertexAttribArray(point_position_id);
    glDisableVertexAttribArray(important_id);
    glDisableVertexAttribArray(selected_id);
    glDisable(GL_DEPTH_TEST);
    
    
    // Done
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


IGL_INLINE void ofxDevelopableViewer::draw_plots()
{
    glBindVertexArray(VAO);
    glUseProgram(plot_prog_id);
    
    //Set color
    GLint elementcolor_loc = glGetUniformLocation(plot_prog_id, "elementcolor");
    glUniform3fv(elementcolor_loc, 1, plotBGColor.data());
    
    //Draw background
    glBindBuffer(GL_ARRAY_BUFFER, plotBBO);
    GLint position_id_plot = glGetAttribLocation(plot_prog_id, "position");
    glVertexAttribPointer(position_id_plot, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(position_id_plot);
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GLenum error = glGetError();
//    if(error != GL_NO_ERROR)
//        std::cout << "There was an OpenGL error while drawing the plot BG. The error is: " << gluErrorString(error) << "." << std::endl;
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisableVertexAttribArray(position_id_plot);
    
    
    //Draw lines
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.);
    
    glBindBuffer(GL_ARRAY_BUFFER, plotLBO);
    glVertexAttribPointer(position_id_plot, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(position_id_plot);
    int offset = 0;
    for(int i=0; i<plotLines.size(); ++i) {
        if(plotLines[i].size()==0)
            continue;
        
        //Color
        if(i<plotColors.size()) {
            glUniform3fv(elementcolor_loc, 1, plotColors[i].data());
        } else {
            glUniform3fv(elementcolor_loc, 1, Eigen::Vector3f(0.1, 0.1, 0.1).data());
        }
        
        glDrawArrays(GL_LINE_STRIP, offset, plotLines[i].size());
//        if(error != GL_NO_ERROR)
//            std::cout << "There was an OpenGL error while drawing line " << i << ". The error is: " << gluErrorString(error) << "." << std::endl;
        
        offset += plotLines[i].size();
    }
    
    glDisable(GL_LINE_SMOOTH);
    
    
#ifdef IGL_VIEWER_WITH_NANOGUI
    //Draw texts
    const double botleftCornerX = 0.05*windowWidth;
    const double botleftCornerY = (0.05+0.2)*windowHeight+3;
    for(int i=0; i<plotTitles.size(); ++i) {
        Eigen::Vector3f color;
        if(i<plotColors.size()) {
            color = plotColors[i];
        } else {
            color = Eigen::Vector3f(0.1, 0.1, 0.1);
        }
        render_text(Eigen::Vector2f(botleftCornerX, botleftCornerY + i*24), plotTitles[i], color);
    }
#endif
    
    
    // Done
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


IGL_INLINE void ofxDevelopableViewer::handle_selection(float x, float y)
{
    if(x<0 || x>windowWidth || y<0 || y>windowHeight)
        return;
    
    if(pointsCoords.rows()==0)
        return;
    
    y = windowHeight - y;
    
    int intx = x;
    int inty = y;
    
    const auto &clearSelected = [this] () {
        for(int j=0; j<pointIsSelected.size(); ++j)
            pointIsSelected(j) = false;
    };
    
    bool selectionMade = false;
    for(int i=0; i<pointsCoords.rows(); ++i) {
        Eigen::Vector3f point = pointsCoords.row(i);
        Eigen::Vector3f point_in_plane = igl::project(point, (view*model).eval(), proj, Eigen::Vector4f(0, 0, windowWidth, windowHeight));
        
        float pointDist = (point_in_plane.segment<2>(0) - Eigen::Vector2f(x, y)).norm();
        const float clickTol = 2.;
        bool pointDistanceSmall = (pointDist < pointSize+clickTol && (!pointIsImportant(i)||!pointIsSelected(i))) || (pointDist < importantPointSize+clickTol && pointIsImportant(i)) || (pointDist < selectedPointSize+clickTol && pointIsSelected(i));
        
        Eigen::Vector3f color;
        glReadPixels(intx, inty, 1, 1, GL_RGB, GL_FLOAT, color.data());
        //To avoid raytracing - a hack with colors to find out if our point is obscured
        const double eps = 2e-6;
        bool pointNotObscured = ((color-pointsFillColor).norm() < eps && (!pointIsImportant(i)||!pointIsSelected(i))) || ((color-pointsImportantColor).norm() < eps && pointIsImportant(i)) || ((color-pointsSelectedColor).norm() < eps && pointIsSelected(i));
        
        if(pointDistanceSmall && pointNotObscured) {
            if(!SHIFT_pressed && !pointIsSelected(i))
                clearSelected();
            
            pointIsSelected(i) = true;
            if(draggingEnabled)
                currentlyDragging = true;
            update_points();
            selectionMade = true;
        }
        
    }
    
    if(!selectionMade) {
        clearSelected();
        update_points();
    }
    
    MOD_happened = true;
}

IGL_INLINE void ofxDevelopableViewer::add_translation_to_selected(const Eigen::Vector3d& diff)
{
    for(int i=0; i<markedPoints.size(); ++i){
        if(pointIsSelected(i)) {
            markedPointsTranslations.row(i) = marked_down_translation.row(i) + diff.transpose();
        }
    }
}


IGL_INLINE void ofxDevelopableViewer::snap()
{
    Eigen::Quaternion<float> snapq;
    igl::snap_to_canonical_view_quat(get_camera_rotation(), 1., snapq);
    set_camera_rotation(snapq);
    update_model_matrix();
}


static void reshape_callback(GLFWwindow* window, int w, int h)
{
    void *data = glfwGetWindowUserPointer(window);
    ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
    viewer->set_window_size(w*viewer->highdpi, h*viewer->highdpi);
    viewer->update_projection_matrix();
}

//static void key_press_callback(GLFWwindow* window, int key, int scancode, int action, int modifier)
//{
//    void *data = glfwGetWindowUserPointer(window);
//    ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
//
//    //Modifiers
////    if(action == GLFW_PRESS) {
////        if(key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT)
////            viewer->SHIFT_pressed = true;
////        else if(key==GLFW_KEY_LEFT_ALT || key==GLFW_KEY_RIGHT_ALT)
////            viewer->ALT_pressed = true;
////        else if(key==GLFW_KEY_LEFT_CONTROL || key==GLFW_KEY_RIGHT_CONTROL)
////            viewer->CTRL_pressed = true;
////    } else {
////        if(key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT)
////            viewer->SHIFT_pressed = false;
////        else if(key==GLFW_KEY_LEFT_ALT || key==GLFW_KEY_RIGHT_ALT)
////            viewer->ALT_pressed = false;
////        else if(key==GLFW_KEY_LEFT_CONTROL || key==GLFW_KEY_RIGHT_CONTROL)
////            viewer->CTRL_pressed = false;
////    }
//
//    //Escape
//    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//        glfwSetWindowShouldClose(window, GL_TRUE);
//
//    //Snap to canonical
//    if(key == GLFW_KEY_S && action == GLFW_PRESS) {
//        viewer->snap();
//    }
//
//    if(viewer->keyboard_callback!=NULL)
//        viewer->keyboard_callback(*viewer, key, scancode, action, modifier);
//}

//static void mouse_move_callback(GLFWwindow* window, double x, double y)
//{
//    void *data = glfwGetWindowUserPointer(window);
//    ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
//
//    viewer->mouse_x = viewer->highdpi*x;
//    viewer->mouse_y = viewer->highdpi*y;
//    static bool nevermoved = true;
//    if(nevermoved) {
//        nevermoved = false;
//        viewer->down_mouse_x = viewer->mouse_x;
//        viewer->down_mouse_y = viewer->mouse_y;
//    }
//
//    if(viewer->LMB_pressed || viewer->MMB_pressed) {
//        //Translate
//        if((viewer->MMB_pressed || (viewer->LMB_pressed && viewer->CTRL_pressed)) && !viewer->get_currently_dragging()) {
//            Eigen::Vector3f pos1 = igl::unproject(Eigen::Vector3f(viewer->mouse_x, viewer->get_height() - viewer->mouse_y, viewer->down_mouse_z), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3f pos0 = igl::unproject(Eigen::Vector3f(viewer->down_mouse_x, viewer->get_height() - viewer->down_mouse_y, viewer->down_mouse_z), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3f diff = pos1 - pos0;
//            viewer->set_camera_translation(viewer->down_translation + diff);
//        }
//        //Rotate
//        else if(viewer->LMB_pressed && !viewer->get_currently_dragging()) {
//            if(viewer->get_rotation_enabled()) {
//                Eigen::Quaternion<float> rot;
//                //two_axis_valuator_fixed_up(viewer->get_width(), viewer->get_height(), 2., down_rot, down_mouse_x, down_mouse_y, mouse_x, mouse_y, rot);
//                igl::trackball(viewer->get_width(), viewer->get_height(), 2., viewer->down_rot, viewer->down_mouse_x, viewer->down_mouse_y, viewer->mouse_x, viewer->mouse_y, rot);
//                viewer->set_camera_rotation(rot);
//            }
//        }
//        //Drag left&right
//        else if(viewer->LMB_pressed && viewer->get_currently_dragging()) {
//            Eigen::Vector3f pos1 = igl::unproject(Eigen::Vector3f(viewer->mouse_x, viewer->get_height() - viewer->mouse_y, viewer->down_mouse_z), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3f pos0 = igl::unproject(Eigen::Vector3f(viewer->down_mouse_x, viewer->get_height() - viewer->down_mouse_y, viewer->down_mouse_z), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3d diff = (pos1 - pos0).cast<double>();
//            viewer->add_translation_to_selected(diff);
//        }
//        //Drag in&out
//        else if((viewer->MMB_pressed || (viewer->LMB_pressed && viewer->CTRL_pressed)) && viewer->get_currently_dragging()) {
//            Eigen::Vector3f pos1 = igl::unproject(Eigen::Vector3f(viewer->down_mouse_x, viewer->get_height() - viewer->down_mouse_y, 0.0005*viewer->mouse_y), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3f pos0 = igl::unproject(Eigen::Vector3f(viewer->down_mouse_x, viewer->get_height() - viewer->down_mouse_y, 0.0005*viewer->down_mouse_y), (viewer->get_view() * viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0, 0, viewer->get_width(), viewer->get_height()));
//            Eigen::Vector3d diff = (pos1 - pos0).cast<double>();
//            viewer->add_translation_to_selected(diff);
//        }
//
//        viewer->update_model_matrix();
//    }
//}

static void mouse_press_callback(GLFWwindow* window, int button, int action, int modifier)
{
    void *data = glfwGetWindowUserPointer(window);
    ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
    
    if(action == GLFW_PRESS) {
        
        viewer->down_mouse_x = viewer->mouse_x;
        viewer->down_mouse_y = viewer->mouse_y;
        Eigen::Vector3f center;
        if (viewer->get_V().rows() == 0)
            center.setZero();
        else
            center = viewer->get_V().colwise().sum()/viewer->get_V().rows();
        Eigen::Vector3f coord = igl::project(center, (viewer->get_view()*viewer->get_model()).eval(), viewer->get_proj(), Eigen::Vector4f(0., 0., viewer->get_width(), viewer->get_height()));
        viewer->down_mouse_z = coord(2);
        //down_mouse_z = 0.;
        viewer->down_rot = viewer->get_camera_rotation();
        viewer->down_translation = viewer->get_camera_translation();
        viewer->marked_down_translation = viewer->get_marked_points_translations();
        
        if(button==GLFW_MOUSE_BUTTON_1)
            viewer->LMB_pressed = true;
        else if(button==GLFW_MOUSE_BUTTON_2)
            viewer->RMB_pressed = true;
        else
            viewer->MMB_pressed = true;
    } else {
        if(button==GLFW_MOUSE_BUTTON_1)
            viewer->LMB_pressed = false;
        else if(button==GLFW_MOUSE_BUTTON_2)
            viewer->RMB_pressed = false;
        else
            viewer->MMB_pressed = false;
    }
    
    //Handle point selection
    if(button==GLFW_MOUSE_BUTTON_1 || button==GLFW_MOUSE_BUTTON_3) {
        if(action==GLFW_PRESS) {
            viewer->handle_selection(viewer->mouse_x, viewer->mouse_y);
        } else {
            viewer->stop_dragging();
        }
    }
}

static void mouse_scroll_callback(GLFWwindow* window, double x, double y)
{
    void *data = glfwGetWindowUserPointer(window);
    ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
    
    float fact = 0.01;
    if(viewer->CTRL_pressed)
        fact *= 0.1;
    
    const float minZoom = 0.01;
    const float maxZoom = 100;
    const float mult = (1. + y*fact);
    const float compdZoom = viewer->get_camera_zoom() * mult;
    if(compdZoom > minZoom) {
        if(compdZoom < maxZoom) {
            viewer->set_camera_zoom(compdZoom);
        } else {
            viewer->set_camera_zoom(maxZoom);
        }
    } else {
        viewer->set_camera_zoom(minZoom);
    }
    
    viewer->update_model_matrix();
}


IGL_INLINE int ofxDevelopableViewer::launch()
{
    launched = true;
    
    //    if(!glfwInit())
    //    {
    //        std::cerr << "Could not initialize glfw" << std::endl;
    //        return EXIT_FAILURE;
    //    }
    const auto & error = [] (int error, const char* description)
    {
        std::cerr << description << std::endl;
    };
    glfwSetErrorCallback(error);
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//    window = glfwCreateWindow(windowWidth, windowHeight, PROJECT_NAME, NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        std::cerr << "Could not create glfw window" << std::endl;
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent(window);
    glEnable(GL_MULTISAMPLE);
    
    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window, this);
    {
        void *data = glfwGetWindowUserPointer(window);
        ofxDevelopableViewer *viewer = static_cast<ofxDevelopableViewer *>(data);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        int width_window, height_window;
        glfwGetWindowSize(window, &width_window, &height_window);
        highdpi = width/width_window;
        reshape_callback(window, width_window, height_window);
    }
    
    // user input callbacks
    glfwSetWindowSizeCallback(window, reshape_callback);
//    glfwSetKeyCallback(window, key_press_callback);
    glfwSetCursorPosCallback(window, mouse_scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_press_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    
    
    // attach shaders and link
    fill_prog_id = start_program(vertex_shader, fragment_shader_fill);
    texture_prog_id = start_program(vertex_shader, fragment_shader_texture);
    lines_prog_id = start_program(vertex_shader, fragment_shader_lines);
    points_prog_id = start_program(points_vertex_shader, points_fragment_shader);
    plot_prog_id = start_program(plot_vertex_shader, plot_fragment_shader);
    
    // Generate and attach buffers to vertex array
    update_mesh_data();
    
    // Colors
    update_colors();
    
    // Lighting
    update_lighting();
    
    // If we have plots, initalize data for them
    if(plottingEnabled)
        update_plots();
    
    // Matrices
    update_projection_matrix();
    update_view_matrix();
    update_model_matrix();
    
    // Main display routine
    start_t = igl::get_seconds();
    last_t = start_t;
    while (!glfwWindowShouldClose(window))
    {
        // Times
        double curr_t = igl::get_seconds();
        double total_t = curr_t - start_t;
        double t = curr_t - last_t;
        
        //Activate window
        //glfwMakeContextCurrent(window);
        
        //Call callback
        if(predraw_callback!=NULL)
            predraw_callback(*this, currentlyDragging||MOD_happened);
        MOD_happened = false;
        // Draw
        drawing();
        if(plottingEnabled)
            draw_plots();
        
        GLfloat lineWidthRange[2] = {0.0f, 0.0f};
        glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, lineWidthRange);
        
        // Anim
        glfwSwapBuffers(window);
        {
            glfwPollEvents();
            // In microseconds
            double duration = 1000000.*(igl::get_seconds()-curr_t);
            const double min_duration = 1000000./60.;
            if(duration<min_duration) {
                std::this_thread::sleep_for(std::chrono::microseconds((int)(min_duration-duration)));
            }
        }
        
        // Times
        last_t = curr_t;
    }
    
    //Shut down
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return EXIT_SUCCESS;
    
}


#ifdef IGL_VIEWER_WITH_NANOGUI
#include <nanovg.h>
#include <igl/viewer/TextRenderer_fonts.h>

#include <Eigen/Dense>

#define NANOVG_GL3
#include <nanovg_gl.h>

IGL_INLINE void ofxDevelopableViewer::render_text(const Eigen::Vector2f& position, const std::string& text)
{
    render_text(position, text, Eigen::Vector3f(0., 0., 0.));
}

IGL_INLINE void ofxDevelopableViewer::render_text(const Eigen::Vector2f& position, const std::string& text, const Eigen::Vector3f& color)
{
    if(!textRendererInitialized) {
        textRendererInitialized = true;
        ctx = (NVGcontext *) nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
        //nvgCreateFontMem((igl::NVGcontext *) ctx, "sans", igl_roboto_regular_ttf, igl_roboto_regular_ttf_size, 0);
        nvgCreateFontMem((igl::NVGcontext *) ctx, "sans", igl_roboto_bold_ttf, igl_roboto_regular_ttf_size, 0);
    }
    Eigen::Vector2i mFBSize;
    Eigen::Vector2i mSize;
    GLFWwindow* mGLFWWindow = glfwGetCurrentContext();
    glfwGetFramebufferSize(mGLFWWindow,&mFBSize[0],&mFBSize[1]);
    glfwGetWindowSize(mGLFWWindow,&mSize[0],&mSize[1]);
    glViewport(0,0,mFBSize[0],mFBSize[1]);
    glClear(GL_STENCIL_BUFFER_BIT);
    /* Calculate pixel ratio for hi-dpi devices. */
    float mPixelRatio = (float)mFBSize[0] / (float)mSize[0];
    
    nvgBeginFrame((igl::NVGcontext *) ctx, mSize[0], mSize[1], mPixelRatio);
    nvgFontSize((igl::NVGcontext *) ctx, 18.0f);
    nvgTextAlign((igl::NVGcontext *) ctx, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
    nvgFillColor((igl::NVGcontext *) ctx, nvgRGBA(255.*color(0), 255.*color(1), 255.*color(2), 255));
    nvgText((igl::NVGcontext *) ctx, position(0)/mPixelRatio, (windowHeight-position(1))/mPixelRatio, text.c_str(), NULL);
    nvgEndFrame((igl::NVGcontext *) ctx);
    
    //nvgDeleteGL3(ctx);
    //textRendererInitialized = false;
}

#endif


IGL_INLINE void ofxDevelopableViewer::exit()
{
    glfwSetWindowShouldClose(window, GL_TRUE);
}


    
