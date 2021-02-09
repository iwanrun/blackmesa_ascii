#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#define screen_width 100
#define screen_height 40
#define max_number_of_3d_points 100000 // lots of available space for the 3d points for the symbol. Might need to be changed depending on the increment value and size
#define M_PI 3.14159265358979323846
const float theta_spacing = 0.01;
int mesh_size;
char output[screen_width][screen_height];
float zbuffer[screen_width][screen_height];
// arrays that start with original_ hold the original pre-rotated coordinates and is populated by the create_symbol function
float original_mesh[max_number_of_3d_points][3];
float original_normals[max_number_of_3d_points][3]; // Store the surface normal of the points in here when creating the shape
float normals[max_number_of_3d_points][3];          // Store the surface normal of the points after rotation
float mesh[max_number_of_3d_points][3];             // Store the mesh after rotation

int camera_pos[] = {0, -1, -25};
float fov_value = 40; // Just an arbitrary value for fov, lower is higher fov
float distance_to_screen = 0;
float font_ratio = 4;       // Change if you want the shape wider or slimmer
float fill_spacing = 0.125; // how much the loop drawing a face is incremented each time
float fps;
float frame_start_time;
float target_fps = 100;
float x_angle, y_angle, z_angle;
float light_pos[] = {5, -5, -5}; // Location of the light which causes shading
float light_direction[3];
//char ascii_palette[] = ".,-~:;=!*#$@";
char ascii_palette[] = ".,-~:;=!?#$&@M";
int basis_vectors[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
void printToScreen()
{
    printf("\x1b[H");
    for (int j = 0; j < screen_height; j++)
    {
        for (int i = 0; i < screen_width; i++)
        {
            /*if (j == screen_height / 2 && i == (screen_width) / 2)
            {
                putchar('*');
            }*/
            char item = output[i][j];
            output[i][j] = 0;
            if (item == 0)
            {
                putchar(' ');
            }
            else
            {

                putchar(item);
            }
        }
        putchar('\n');
    }
}

void normaliseAndPlaceVector(float x, float y, float z, int counter)
{
    float magnitude = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    original_normals[counter][0] = x / magnitude;
    original_normals[counter][1] = y / magnitude;
    original_normals[counter][2] = z / magnitude;
}

void createSymbol()
{
    int closest_z = -1;
    float circle_width = 2;
    int counter = 0;
    int size = 8;
    int height_of_ridge = 0 - size / 5;
    float gradient = -0.1;
    float displacement = size / 2;
    float z_fighting_offset = 0.2;
    for (float depth = closest_z; depth <= -closest_z; depth += fill_spacing) // The encapsulating loop for depth
    {
        if (depth == closest_z || depth == -closest_z) // only passes if doing the closes or furthest z value
        {
            for (float i = 0; i < circle_width / 2; i += fill_spacing) // draws the circle faces
            {
                for (float theta = 0; theta < (M_PI * 2); theta += theta_spacing)
                {
                    float x = cos(theta) * (size + i);
                    float y = sin(theta) * (size + i);
                    original_mesh[counter][0] = x;
                    original_mesh[counter][1] = y;
                    original_mesh[counter][2] = depth;
                    if (i == 0)
                    {
                        normaliseAndPlaceVector(-x, -y, 0, counter); // inside circle
                    }
                    else if (i == 1)
                    {
                        normaliseAndPlaceVector(x, y, 0, counter); // outside circle
                    }
                    else // must be part of the middle section, which either faces towards or away from the camera initially
                    {
                        normaliseAndPlaceVector(0, 0, depth, counter);
                    }
                    counter++;
                }
            }
        }
        else
        {
            for (float i = 0; i < circle_width; i += circle_width / 2) // Just draw the inside and outside of the cyclinder
            {
                for (float theta = 0; theta < (M_PI * 2); theta += theta_spacing)
                {
                    float x = cos(theta) * (size + i);
                    float y = sin(theta) * (size + i);
                    original_mesh[counter][0] = x;
                    original_mesh[counter][1] = y;
                    original_mesh[counter][2] = depth;
                    if (i == 0)
                    {
                        normaliseAndPlaceVector(-x, -y, 0, counter); // inside circle
                    }
                    else
                    {
                        normaliseAndPlaceVector(x, y, 0, counter); // outside circle
                    }
                    counter++;
                }
            }
        }
        float y_value_of_solid_fill = sin(4) * size;

        if (depth == closest_z || depth == -closest_z) // do either closest or furthest face, only at start and end of z loop so it is hollow.
        {
            // traverse bottom up, left to right, this is the curved solid fill
            for (float row = -size - z_fighting_offset; row < y_value_of_solid_fill + size / 4; row += fill_spacing) // bottom up
            {
                float left_most_intercept = -sqrt(pow(size, 2) - pow(row, 2));
                left_most_intercept -= z_fighting_offset;
                for (float column = left_most_intercept; column < -left_most_intercept; column += fill_spacing) // left to right
                {
                    if (row > y_value_of_solid_fill && column > -(size / 2))
                    {
                        original_mesh[counter][0] = column;
                        original_mesh[counter][1] = row;

                        if (depth == closest_z) // to prevent z fighting
                        {
                            original_mesh[counter][2] = depth - z_fighting_offset;
                        }
                        else if (depth == -closest_z)
                        {
                            original_mesh[counter][2] = depth + z_fighting_offset;
                        }
                        normaliseAndPlaceVector(0, 0, depth, counter);
                        counter++;
                    }
                    else if (row < y_value_of_solid_fill)
                    {
                        original_mesh[counter][0] = column;
                        original_mesh[counter][1] = row;
                        if (depth == closest_z) // to prevent z fighting
                        {
                            original_mesh[counter][2] = depth - z_fighting_offset;
                        }
                        if (depth == -closest_z)
                        {
                            original_mesh[counter][2] = depth + z_fighting_offset;
                        }
                        normaliseAndPlaceVector(0, 0, depth, counter);
                        counter++;
                    }
                }
            }
            // |----\ section of the symbol from height of ridge down to where the angled ramp intercepts the inner circle (that point is based on guesstimation)
            int row_num = 0;
            for (float row = height_of_ridge; row > y_value_of_solid_fill + size / 4; row -= fill_spacing)
            {
                row_num++;
                for (float column = -(size / 2); column < (-gradient * row_num + displacement); column += fill_spacing)
                {
                    original_mesh[counter][0] = column;
                    original_mesh[counter][1] = row;
                    original_mesh[counter][2] = depth;
                    normaliseAndPlaceVector(0, 0, depth, counter);
                    counter++;
                }
            }
        }
        else
        {
            // ramp section lid
            int row_num = 0;
            for (float row = height_of_ridge; row > y_value_of_solid_fill + size / 4; row -= fill_spacing)
            {
                row_num++;
                original_mesh[counter][0] = displacement + (-gradient * row_num);
                original_mesh[counter][1] = row;
                original_mesh[counter][2] = depth;
                normaliseAndPlaceVector(1, gradient, 0, counter);
                counter++;
            }

            // top flat lid
            for (float column = -(size / 2) + 0.1; column < displacement; column += fill_spacing)
            {
                original_mesh[counter][0] = column;
                original_mesh[counter][1] = height_of_ridge;
                original_mesh[counter][2] = depth;
                normaliseAndPlaceVector(0, 1, 0, counter);
                counter++;
            }
            // vertical wall
            for (float row = y_value_of_solid_fill; row < height_of_ridge; row += fill_spacing)
            {
                original_mesh[counter][0] = -(size / 2);
                original_mesh[counter][1] = row;
                original_mesh[counter][2] = depth;
                normaliseAndPlaceVector(-1, 0, 0, counter);
                counter++;
            }
            // left most flat lid
            for (float column = -sqrt(pow(size, 2) - pow(y_value_of_solid_fill, 2)); column < -(size / 2); column += fill_spacing)
            {
                original_mesh[counter][0] = column;
                original_mesh[counter][1] = y_value_of_solid_fill;
                original_mesh[counter][2] = depth;
                normaliseAndPlaceVector(0, 1, 0, counter);
                counter++;
            }
        }
    }
    mesh_size = counter;
}

void rotationSystem(float a, float b, float c)
{

    x_angle = fmod(x_angle + a, 360);
    y_angle = fmod(y_angle + b, 360);
    z_angle = fmod(z_angle + c, 360);
    /*x_angle = fmod(a, 360);
    y_angle = fmod(b, 360);
    z_angle = fmod(c, 360);*/
}

void rotate()
{
    float pi_over_180 = M_PI / 180;
    float a = z_angle * pi_over_180;
    float b = y_angle * pi_over_180;
    float c = x_angle * pi_over_180;
    //https://en.wikipedia.org/wiki/Rotation_matrix

    /*float R[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}};*/

    float R[3][3] = {
        {cos(a) * cos(b), cos(a) * sin(b) * sin(c) - sin(a) * cos(c), cos(a) * sin(b) * cos(c) + sin(a) * sin(c)},
        {sin(a) * cos(b), sin(a) * sin(b) * sin(c) + cos(a) * cos(c), sin(a) * sin(b) * cos(c) - cos(a) * sin(c)},
        {-sin(b), cos(b) * sin(c), cos(b) * cos(c)}};

    //printf("%f,%f,%f \n", R[0][0], R[0][1], R[0][2]);
    //printf("%f,%f,%f \n", R[1][0], R[1][1], R[1][2]);
    //printf("%f,%f,%f \n", R[2][0], R[2][1], R[2][2]);
    for (int i = 0; i < mesh_size; i++) // go through mesh
    {
        // multiply with matrix (where j is each row top to bottom)
        for (int j = 0; j < 3; j++)
        {
            mesh[i][j] = R[j][0] * original_mesh[i][0] + R[j][1] * original_mesh[i][1] + R[j][2] * original_mesh[i][2];
        }
    }
    // calculate rotated normals
    for (int i = 0; i < mesh_size; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            normals[i][j] = R[j][0] * original_normals[i][0] + R[j][1] * original_normals[i][1] + R[j][2] * original_normals[i][2];
        }
    }
}

float dotProduct(float vec1[3], float vec2[3])
{
    return (vec1[0] * vec2[0] + vec1[1] * vec2[1] + vec1[2] * vec2[2]);
}

void renderFrame() // Puts points on the z plane and makes them ints
{
    // undergo perspective projection to put everything on a z plane.
    for (int i = 0; i < mesh_size; i++)
    {
        // camera transform, doesn't support camera rotation
        float vector[] = {mesh[i][0] - camera_pos[0], mesh[i][1] - camera_pos[1], mesh[i][2] - camera_pos[2]};
        // projection, maybe wrong
        //https://gamedev.stackexchange.com/questions/103693/why-do-we-need-a-fourth-coordinate-to-divide-by-z
        // seems like there are many ways to do it
        vector[0] = (vector[0] * fov_value) / (vector[2] + distance_to_screen);
        vector[1] = (vector[1] * fov_value) / (vector[2] + distance_to_screen);
        //vector[2] = vector[2] / vector[2];
        int x = round(((vector[0] * font_ratio) + screen_width) / 2);
        int y = round((-1 * vector[1]) + (screen_height / 2)); // flip y axis as the y axis is flipped on a screen
        bool in_screen = x < screen_width && x > 0 && y > 0 && y < screen_height && mesh[i][2] > (camera_pos[2] + distance_to_screen);
        if (in_screen)
        {
            // pixel is on screen
            if (mesh[i][2] < zbuffer[x][y])
            {
                int luminance_index = round(13 * dotProduct(normals[i], light_direction)); // ranges from 0 to 13
                if (luminance_index < 0)
                {
                    luminance_index = 0;
                }
                output[x][y] = ascii_palette[luminance_index];
                zbuffer[x][y] = mesh[i][2];
            }
        }
    }
}

void initialiseZbuffer()
{
    for (int i = 0; i < screen_width; i++)
    {
        for (int j = 0; j < screen_height; j++)
        {
            zbuffer[i][j] = 50; // the z value of the background
        }
    }
}
void lightSetup()
{
    // normalise the vector to the light
    float magnitude = sqrt(light_pos[0] * light_pos[0] + light_pos[1] * light_pos[1] + light_pos[2] * light_pos[2]);
    light_direction[0] = light_pos[0] / magnitude;
    light_direction[1] = light_pos[1] / magnitude;
    light_direction[2] = light_pos[2] / magnitude;
}
void manageTime()
{
    float current_time = clock() / CLOCKS_PER_SEC;
    float delta_time = current_time - frame_start_time;
    fps = 1 / (delta_time + (1000 / target_fps) / 1000); // the denominator is the time it took to get the frame. The division asks how many times that can be done a second.
    frame_start_time = current_time;
}
int main()
{
    createSymbol();
    //usleep(100000000);
    lightSetup();
    frame_start_time = clock() / CLOCKS_PER_SEC;
    for (;;)
    {
        rotationSystem(0, 0.6, 0); // x,y, or z axis to rotate around
        rotate();
        initialiseZbuffer();
        renderFrame();
        printToScreen();
        manageTime();
        printf("mesh size: %d fps: %.1f \n", mesh_size, fps);
        usleep((1000 / target_fps) * 1000);
    }
}
