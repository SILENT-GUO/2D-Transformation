#include "MainFrame.h"
#include <iostream>

namespace {

float scale = 1.f;
float aspect = 1.f;

#ifdef __APPLE__
unsigned int SCR_WIDTH = 600;
unsigned int SCR_HEIGHT = 600;
#else
unsigned int SCR_WIDTH = 1000;
unsigned int SCR_HEIGHT = 1000;
#endif

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    scale *= std::pow(1.1f, (float)yoffset);
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);        // Set the viewport to cover the new window                  
    aspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;  // Set the aspect ratio of the clipping area to match the viewport
}

}

void MainFrame::LeftMouseMove(float start_x, float start_y, float curr_x, float curr_y) {
    if (modeling_state_ == OBJ_ROTATION) {
        // ---------------------------------- Object Rotation ---------------------------------------
        // TODO: Add your code here.
        // Find the correct 4x4 transform matrix "transform_mat" to rotate the object about its center.

        glm::mat4x4 transform_mat(1.f);
        //first find the rotation axis and angle
        glm::vec2 V = glm::vec2(curr_x - start_x, curr_y - start_y);
        glm::vec2 A = glm::vec2(-V.y, V.x);
        //screen to world
        glm::vec3 rot_axis = glm::normalize(Screen2World(A + glm::vec2(start_x, start_y)) - Screen2World(glm::vec2(start_x, start_y)));
        float angle = glm::length(A) * 0.007f;
        //then construct the rotation matrix
        transform_mat = glm::rotate(glm::mat4x4(1.f), angle, rot_axis);
        // construct the entire transformation matrix
        //first translate the object to the origin
        glm::vec3 translation = -mesh_.center_;
        //then rotate the object
        transform_mat = glm::translate(glm::mat4x4(1.f), translation) * transform_mat * glm::translate(glm::mat4x4(1.f), -translation);
        mesh_.ApplyTransform(transform_mat);
    }
    else if (modeling_state_ == OBJ_TRANSLATION) {
        // ---------------------------------- Object Translation ------------------------------------
        // TODO: Add your code here.
        // Find the correct 4x4 transform matrix "trans_mat" to translate the object along the view plane.

        glm::mat4x4 transform_mat(1.f);
        //first find p_start and p_cur from start_x, start_y, curr_x, curr_y
        //Pstart is the intersected point of the object and the ray 𝑅 ଴that goes through Sstart.
        //Pcur is the intersected point of the object and the ray 𝑅 ଴that goes through Scur.
        //then create rays using Screen2WorldRay function
        std::tuple<glm::vec3, glm::vec3> ray_start = Screen2WorldRay(start_x, start_y);
        std::tuple<glm::vec3, glm::vec3> ray_cur = Screen2WorldRay(curr_x, curr_y);
        //then find the intersection points of the rays and the object
        std::tuple<int, glm::vec3> face_start = mesh_.FaceIntersection(std::get<0>(ray_start), std::get<1>(ray_start));
        std::tuple<int, glm::vec3> face_cur = mesh_.FaceIntersection(std::get<0>(ray_cur), std::get<1>(ray_cur));

        glm::vec3 p_start = std::get<1>(face_start);
        glm::vec3 p_cur = std::get<1>(face_cur);
        //z value seems not equal unless we assign the same direction value.
        p_start = Screen2World(start_x, start_y, -std::get<0>(ray_start).z + std::get<1>(ray_start).z);
        p_cur = Screen2World(curr_x, curr_y, -std::get<0>(ray_cur).z + std::get<1>(ray_cur).z);
        //construct the translation matrix
        glm::vec3 translation = p_cur - p_start;
        transform_mat = glm::translate(glm::mat4x4(1.f), translation);
        mesh_.ApplyTransform(transform_mat);

    }
    else if (modeling_state_ == OBJ_EXTRUDE) {
        // ---------------------------------- Face Extrusion ------------------------------------
        // TODO: Add your code here.
        // Find the correct 4x4 transform matrix "trans_mat" to translate the face vertices along the face normal.
 
        glm::mat4x4 transform_mat(1.f);
        int face_index = -1;
        //first find the face index
        std::tuple<glm::vec3, glm::vec3> ray_start = Screen2WorldRay(start_x, start_y);
        std::tuple<glm::vec3 , glm::vec3> ray_cur = Screen2WorldRay(curr_x, curr_y);
        std::cout << "ray_cur<1>: " << std::get<1>(ray_cur).x << " " << std::get<1>(ray_cur).y << " " << std::get<1>(ray_cur).z << std::endl;
        std::tuple<int, glm::vec3> face_start = mesh_.FaceIntersection(std::get<0>(ray_start), std::get<1>(ray_start));
        face_index = std::get<0>(face_start);
        glm::vec3 p_start = std::get<1>(face_start);
        std::cout << "p_start: " << p_start.x << " " << p_start.y << " " << p_start.z << std::endl;
        //then find the face normal, calculate by cross product
        glm::vec3 face_normal = glm::normalize(glm::cross(mesh_.vertices_[mesh_.faces_[face_index][1]] - mesh_.vertices_[mesh_.faces_[face_index][0]], mesh_.vertices_[mesh_.faces_[face_index][2]] - mesh_.vertices_[mesh_.faces_[face_index][0]]));
        //we can represent ray M = Pstart + t * face_normal
//        glm::vec3 M = p_start + 1.0f * face_normal;
//        std::cout << "M " << M.x << " " << M.y << " " << M.z << std::endl;
        //then find the shortest distance between the ray and the object by cross product of M and ray_cur
        glm::vec3 V = glm::normalize(glm::cross(face_normal, std::get<1>(ray_cur)));
        std::cout << "V " << V.x << " " << V.y << " " << V.z << std::endl;
        //then represent the plane Q from V and ray_cur by a point on the V and normal vector
        glm::vec3 pt_on_plane = std::get<0>(ray_cur);
        glm::vec3 normal_vector = glm::cross(std::get<1>(ray_cur), V );
        //(A,B,C) is M, find D
        float D = -glm::dot(pt_on_plane, normal_vector);
        std::cout << "A: " << normal_vector.x << "B: " << normal_vector.y << "C: " << normal_vector.z << "D: " << D << std::endl;
        //then find the intersection point of the ray and the plane
        //solve t

        float t = (-D - normal_vector.z * p_start.z - normal_vector.y * p_start.y - normal_vector.x * p_start.x) / (normal_vector.z * face_normal.z + normal_vector.y * face_normal.y + normal_vector.x * face_normal.x);
        //then find the intersection point
        glm::vec3 p_cur = p_start + t * face_normal;
        std::cout << "t: " << t << std::endl;
        std::cout << "p_cur: " << p_cur.x << " " << p_cur.y << " " << p_cur.z << std::endl;
        //then find the translation vector
        glm::vec3 translation = p_cur - p_start;
        if(glm::abs(translation.x) < 1e-5){
            translation.x = 0;
        }
        if(glm::abs(translation.y) < 1e-5){
            translation.y = 0;
        }
        if(glm::abs(translation.z) < 1e-5){
            translation.z = 0;
        }
        //construct the translation matrix
        transform_mat = glm::translate(glm::mat4x4(1.f), translation);
        mesh_.ApplyFaceTransform(face_index, transform_mat);
    }
}

void MainFrame::VisualizeWorldSpace() {
    // ---------------------------------- World Space Visualization ------------------------------------
    // TODO: Add your code here to visualize the world space.

    glBegin(GL_LINES);
    glColor3f(1.f,0.0f,0.0f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(3.0f,0.0f,0.0f);

    glColor3f(0.f,1.f,0.f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(0.0f,3.0f,0.0f);

    glColor3f(0.f,0.f,1.f);
    glVertex3f(0.0f,0.0f,0.0f);
    glVertex3f(0.0f,0.0f,3.0f);

    glEnd();
    // add x-y plane visualization
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i++) {
        glColor3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-10.0f, i, 0.0f);
        glVertex3f(10.0f, i, 0.0f);
        glVertex3f(i, -10.0f, 0.0f);
        glVertex3f(i, 10.0f, 0.0f);
    }
    glEnd();
    // ...
}



// -------------------------------------------------------------------------------------
// -------------------------- No need to change ----------------------------------------
// -------------------------------------------------------------------------------------
void MainFrame::MainLoop() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);

    // glfw window creation, set viewport with width=1000 and height=1000
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3DModeling", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    // glad: load all OpenGL function pointers
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    const float alpha = 0.3f;
    const float beta = 0.1f;

    const float r = 5.f;
    camera_.LookAt(r * glm::vec3(std::cos(alpha) * std::cos(beta), std::cos(alpha) * std::sin(beta), std::sin(alpha)),
        glm::vec3(0.f, 0.f, 0.f),
        glm::vec3(0.f, 0.f, 1.f));

    glEnable(GL_DEPTH_TEST);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        ProcessInput(window);

        // glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Apply camera projection;
        camera_.Perspective(90.f, aspect, .5f, 10.f);
        camera_.UpdateScale(scale);
        scale = 1.f;
        camera_.ApplyProjection();

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        // Clear the display

        DrawScene();
        VisualizeWorldSpace();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // glfw: terminate, clearing addl previously allocated GLFW resources.
    glfwTerminate();
}

void MainFrame::ProcessInput(GLFWwindow* window) {
    // Key events
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        modeling_state_ = OBJ_ROTATION;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        modeling_state_ = OBJ_TRANSLATION;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        modeling_state_ = OBJ_SUBDIVIDE;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        modeling_state_ = OBJ_EXTRUDE;
    }
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    int current_l_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Handle left mouse
    if (current_l_mouse_state == GLFW_PRESS) {
        double xposd, yposd;
        float xpos, ypos;
        glfwGetCursorPos(window, &xposd, &yposd);
        xpos = float(xposd);
        ypos = float(SCR_HEIGHT - yposd);
        if (l_mouse_state_ == GLFW_RELEASE) {
            LeftMouseClick(xpos, ypos);
            l_click_cursor_x_ = xpos;
            l_click_cursor_y_ = ypos;
        }
        if (l_mouse_state_ == GLFW_PRESS &&
            (std::abs(xpos - last_cursor_x_) > 2.f || std::abs(ypos - last_cursor_y_) > 2.f)) {
            LeftMouseMove(l_click_cursor_x_, l_click_cursor_y_, xpos, ypos);
        }
        last_cursor_x_ = float(xpos);
        last_cursor_y_ = float(ypos);
    }
    if (current_l_mouse_state == GLFW_RELEASE) {
        if (l_mouse_state_ == GLFW_PRESS) {
            LeftMouseRelease();
        }
    }
    l_mouse_state_ = current_l_mouse_state;

    // Handle right mouse
    int current_r_mouse_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (current_r_mouse_state == GLFW_PRESS) {
        double xposd, yposd;
        float xpos, ypos;
        glfwGetCursorPos(window, &xposd, &yposd);
        xpos = float(xposd);
        ypos = float(SCR_HEIGHT - yposd);
        if (r_mouse_state_ == GLFW_RELEASE) {
            RightMouseClick(xpos, ypos);
        }
        if (r_mouse_state_ == GLFW_PRESS &&
            (std::abs(xpos - last_cursor_x_) > 2.f || std::abs(ypos - last_cursor_y_) > 2.f)) {
            RightMouseMove(last_cursor_x_, last_cursor_y_, xpos, ypos);
        }
        last_cursor_x_ = float(xpos);
        last_cursor_y_ = float(ypos);
    }
    if (current_r_mouse_state == GLFW_RELEASE) {
        if (r_mouse_state_ == GLFW_PRESS) {
            RightMouseRelease();
        }
    }
    r_mouse_state_ = current_r_mouse_state;
}

void MainFrame::LeftMouseClick(float x, float y) {
    if (modeling_state_ == OBJ_SUBDIVIDE) {
        glm::vec3 p_world = Screen2World(x, y);
        glm::vec3 cam_pos = camera_.view_mat_inv_ * glm::vec4(0.f, 0.f, 0.f, 1.f);
        mesh_.SubdivideFace(cam_pos, glm::normalize(p_world - cam_pos));
    }
    else if (modeling_state_ == OBJ_EXTRUDE) {
        glm::vec3 p_world = Screen2World(x, y);
        glm::vec3 cam_pos = camera_.view_mat_inv_ * glm::vec4(0.f, 0.f, 0.f, 1.f);
        mesh_.GenExtrudeFace(cam_pos, glm::normalize(p_world - cam_pos));
    }
}

void MainFrame::LeftMouseRelease() {
    mesh_.CommitTransform();
}

void MainFrame::RightMouseClick(float x, float y) {
    return;
}

void MainFrame::RightMouseMove(float start_x, float start_y, float curr_x, float curr_y) {
    glm::vec2 s_start(start_x, start_y);
    glm::vec2 s_cur(curr_x, curr_y);
    glm::vec2 V = s_cur - s_start;
    glm::vec2 A = glm::vec2(-V.y, V.x);
    glm::vec3 rot_axis = glm::normalize(Screen2World(A + s_start) - Screen2World(s_start));
    glm::mat4x4 rot_mat = glm::rotate(glm::mat4x4(1.f), 0.007f * glm::length(A), rot_axis);
    camera_.ApplyTransform(rot_mat);
}

void MainFrame::RightMouseRelease() {
    return;
}

glm::vec3 MainFrame::Camera2World(const glm::vec3& x, float w) {
    return glm::vec3(camera_.view_mat_inv_ * glm::vec4(x, w));
}

glm::vec3 MainFrame::World2Camera(const glm::vec3& x, float w) {
    return glm::vec3(camera_.view_mat_ * glm::vec4(x, w));
}

glm::vec3 MainFrame::Screen2World(const glm::vec2& v, float depth) {
    float x = v.x / SCR_WIDTH  * 2.f - 1.f;
    float y = v.y / SCR_HEIGHT * 2.f - 1.f;
    float focal = std::tan(camera_.fov_ * .5f / 180.f * glm::pi<float>());
    glm::vec4 v_camera(x * focal * aspect, y * focal, -1.f, 1.f);
    v_camera = v_camera * depth;
    glm::vec4 v_world = camera_.view_mat_inv_ * v_camera;
    return glm::vec3(v_world);
}

glm::vec3 MainFrame::Screen2World(float scr_x, float scr_y, float camera_z) {
    float x = scr_x / SCR_WIDTH * 2.f - 1.f;
    float y = scr_y / SCR_HEIGHT * 2.f - 1.f;
    float focal = std::tan(camera_.fov_ * .5f / 180.f * glm::pi<float>());
    glm::vec4 v_camera(x * focal * aspect, y * focal, -1.f, 1.f);
    v_camera = v_camera * -camera_z;
    glm::vec4 v_world = camera_.view_mat_inv_ * v_camera;
    return glm::vec3(v_world);
}

std::tuple<glm::vec3, glm::vec3> MainFrame::Screen2WorldRay(float scr_x, float scr_y) {
    float x = scr_x / SCR_WIDTH * 2.f - 1.f;
    float y = scr_y / SCR_HEIGHT * 2.f - 1.f;
    float focal = std::tan(camera_.fov_ * .5f / 180.f * glm::pi<float>());
    glm::vec3 o = camera_.view_mat_inv_ * glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::vec4 v_camera(x * focal * aspect, y * focal, -1.f, 0.f);
    glm::vec3 v = camera_.view_mat_inv_ * v_camera;
    return std::make_tuple(o, v);
}

void MainFrame::DrawScene() {
    // Draw mesh
    mesh_.Draw();

    VisualizeWorldSpace();
}