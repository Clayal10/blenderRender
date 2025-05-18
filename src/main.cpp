#define MAIN
#include "helpers.h"

#include<fstream>
#include<sstream>

std::mutex grand_mutex;

float height = 1600;
float width = 2550;

/* Global section */
int time_resolution = 10;
int framecount = 0;
std::vector<gameobject*> objects;


struct key_status {
	int forward, backward, left, right, up, down;
};
struct key_status player_key_status;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	
	if(GLFW_KEY_W == key && 1 == action){
		player_key_status.forward = 1;
	}	
	else if(GLFW_KEY_W == key && 0 == action){
		player_key_status.forward = 0;
	}	
	if(GLFW_KEY_S == key && 1 == action){
		player_key_status.backward = 1;
	}	
	else if(GLFW_KEY_S == key && 0 == action){
		player_key_status.backward = 0;
	}
	if(GLFW_KEY_A == key)
		player_key_status.left = action;
	if(GLFW_KEY_D == key)
		player_key_status.right = action;
	/*for the game, will want to disable jumping and running, but will be fine through project 4 probably*/
	
	if(GLFW_KEY_SPACE == key){	
		
		if(player_platform || player_position.y < 0.1f + player_height){
			player_fly = 0;
			//player_fall_speed = 0.04f;
			player_fall_speed = 0.2f;

			player_position.y += 1.0f;
			player_platform = 0;
		}
	} // may need to check and make sure shift and space aren't pressed at the same time
	if(GLFW_KEY_LEFT_CONTROL == key){
		player_key_status.up = action;
		player_fall_speed = 0.0f;
		player_fly = 1;
	}
	if(GLFW_KEY_LEFT_SHIFT == key){
		if(!player_platform)
			player_key_status.down = action;
	}
}

int shutdown_engine = 0;
/* Must be called at a consistent rate */
void player_movement(){
	while(!shutdown_engine){
		//		grand_mutex.lock();
		auto start = std::chrono::system_clock::now();
		glm::vec3 step_to_point = player_position;
		if(player_key_status.forward){
			step_to_point += 0.1f * glm::vec3(sinf(player_heading), 0, cosf(player_heading));
			//step_to_point += 0.6f * glm::vec3(sinf(player_heading), 0, cosf(player_heading));

		}
		if(player_key_status.backward){
			step_to_point += 0.1f * glm::vec3(-sinf(player_heading), 0, -cosf(player_heading));
			//step_to_point += 0.6f * glm::vec3(-sinf(player_heading), 0, -cosf(player_heading));

		}
		if(player_key_status.left){
			step_to_point += 0.05f * glm::vec3(sinf(player_heading + M_PI/2), 0, cosf(player_heading + M_PI/2));
			//step_to_point += 0.4f * glm::vec3(sinf(player_heading + M_PI / 2), 0, cosf(player_heading + M_PI / 2));

		}
		if(player_key_status.right){
			step_to_point += 0.05f * glm::vec3(-sinf(player_heading + M_PI/2), 0, -cosf(player_heading + M_PI/2));
			//step_to_point += 0.4f * glm::vec3(-sinf(player_heading + M_PI / 2), 0, -cosf(player_heading + M_PI / 2));

		}
		if(player_key_status.up){
			step_to_point += 0.05f * glm::vec3(0, 1, 0);
			//step_to_point += 0.3f * glm::vec3(0, 1, 0);

		}
		if(player_key_status.down){
			step_to_point += 0.05f * glm::vec3(0, -1, 0);
			//step_to_point += 0.3f * glm::vec3(0, -1, 0);

		}

		//		grand_mutex.unlock();
		auto end = std::chrono::system_clock::now();
		//		double difference = std::chrono::duration_cast<std::chrono::milliseconds>(start - end).count();
		//		printf("Time difference:  %lf\n", difference);
		std::this_thread::sleep_for(std::chrono::microseconds(1000) - (start - end));
	}
}

void object_movement(){
	int loop_time;
	auto last_call = std::chrono::system_clock::now(); 
	while(!shutdown_engine){
		auto start_time = std::chrono::system_clock::now();
		loop_time = std::chrono::duration_cast<std::chrono::microseconds>(start_time - last_call).count();

		//		grand_mutex.lock();
		if(player_platform){
			glm::vec3 pltloc = player_platform->locations[player_platform_index];
			float floor_height = pltloc.y + (player_platform->size.y / 2);
			player_position.y = floor_height + player_height;
		}
		//		grand_mutex.unlock();
		for(gameobject* o : objects)
			o->move(loop_time); // move needs a parameter which indicates how long since we last called move (loop_time)
		auto end_time = std::chrono::system_clock::now();
		int cpu_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
		last_call = start_time;
		int sleep_time = 1000 - cpu_time;
		//		printf("Loop time:  %d		Sleep time:  %d	CPU time:  %d\n", loop_time, sleep_time, cpu_time);
		if(sleep_time > 100 )
			std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
	}
}

void animation(){
	while(!shutdown_engine){
		auto start = std::chrono::system_clock::now();
		for(gameobject* o : objects)
			o->animate();
		auto end = std::chrono::system_clock::now();
		//		double difference = std::chrono::duration_cast<std::chrono::milliseconds>(start - end).count();
		//		printf("Time difference:  %lf\n", difference);
		std::this_thread::sleep_for(std::chrono::microseconds(10000) - (start - end));
	}
}


void pos_callback(GLFWwindow* window, double xpos, double ypos){
	double center_x = width/2;
	double diff_x = xpos - center_x;
	double center_y = height/2;
	double diff_y = ypos - center_y;
	glfwSetCursorPos(window, center_x, center_y);
	player_heading -= diff_x / 1000.0; // Is this too fast or slow?
	player_elevation -= diff_y / 1000.0;
}

void resize(GLFWwindow*, int new_width, int new_height){
	width = new_width;
	height = new_height;
	printf("Window resized, now %f by %f\n", width, height);
	glViewport(0, 0, width, height);
}

void debug_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
	if(severity == GL_DEBUG_SEVERITY_HIGH)
		puts(RED(message).c_str());
	else if(severity == GL_DEBUG_SEVERITY_MEDIUM)
		puts(YELLOW(message).c_str());
	/* Only uncomment if you want a lot of messages */
	/*
		else
		puts(message);
		*/
}


int main(int argc, char** argv) {
	init_helpers();
	if(!glfwInit()) {
		puts(RED("GLFW init error, terminating\n").c_str());
		return 1;
	}
	GLFWwindow* window = glfwCreateWindow(width, height, "Simple OpenGL 4.0+ Demo", 0, 0);
	if(!window){
		puts(RED("Window creation failed, terminating\n").c_str());
		return 1;
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(window);
	glewInit();

	unsigned supported_threads = std::thread::hardware_concurrency();
	printf("Supported threads:  %u\n", supported_threads);

	/* Set up callbacks */
	glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(debug_callback, 0);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, pos_callback);
	glfwSetFramebufferSizeCallback(window, resize);

	/* Set starting point */
	player_position = glm::vec3(10, 10, 500);
	player_heading = M_PI;
	/* Initialize game objects */
	for(gameobject* o : objects){
		if(o->init()){
			puts(RED("Compile Failed, giving up!").c_str());
			return 1;
		}
	}

	/* Start Other Threads */
	std::thread player_movement_thread(player_movement);
	std::thread object_movement_thread(object_movement);
	std::thread animation_thread(animation);

	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window)) {
		framecount++;
		glfwPollEvents();
		glClearColor(0, 0, 0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		//		grand_mutex.lock();

		/* Where are we?  A:  player_position
		 * What are we looking at?
		 */
		glm::vec3 look_at_point = player_position;
		look_at_point.x += cosf(player_elevation) * sinf(player_heading);
		look_at_point.y += sinf(player_elevation);
		look_at_point.z += cosf(player_elevation) * cosf(player_heading);
		glm::mat4 view = glm::lookAt(player_position, look_at_point, glm::vec3(0, 1, 0));
		glm::mat4 projection = glm::perspective(45.0f, width / height, 0.1f, 10000.0f);
		glm::mat4 vp = projection * view;

		for(gameobject* o : objects)
			o->draw(vp);
		//		grand_mutex.unlock();

		glfwSwapBuffers(window);
	}
	shutdown_engine = 1;
	player_movement_thread.join();
	// TODO:  join other threads
	glfwDestroyWindow(window);
	glfwTerminate();
	free_helpers();
}


