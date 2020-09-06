#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * KillerPongMode is a game mode that implements a single-player game of Pong.
 */

struct KillerPongMode : Mode {
    KillerPongMode();
	virtual ~KillerPongMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	bool is_complete();

	//----- game state -----
	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
    glm::vec2 paddle_radius = glm::vec2(0.2f, 1.0f);
	glm::vec2 ball_radius = glm::vec2(0.2f, 0.2f);

	glm::vec2 left_paddle = glm::vec2(-court_radius.x + 0.5f, 0.0f);
	glm::vec2 right_paddle = glm::vec2( court_radius.x - 0.5f, 0.0f);

    // initially every one has 20 hps, if got hit by the ball, reduce 1 hp
	uint32_t left_hp = 15;
	uint32_t right_hp = 15;
	// when losing 1 hp, there are invincible_sec seconds for invincible
	float invincible_sec = 1.0f;
    float left_invincible_elapsed = 0.0f;
    float right_invincible_elapsed = 0.0f;
    // how fast can this ai move (the larger the harder to beat the ai)
	float ai_speed_factor = 7.0f;
	// 1: currently moving up, -1: moving down, 0: not moving
	int ai_moving_direction = 0;

	// time length (in seconds) of the ball trail
    static constexpr float trail_length = 0.1f;
    // create a new ball every 5 secs
    static constexpr float ball_create_interval = 5.0f;

    // increase speed of each ball by ball_speed_inc_ratio (%) every ball_speed_inc_interval (s) seconds
    static constexpr float ball_speed_inc_ratio = 0.3f;
    static constexpr float ball_speed_inc_interval = 1.0f;
    // allow max 10 times faster than init speed (the larger the harder for the game)
    static constexpr float ball_speed_max_multiplier = 10.0f;

    // Represents a single ball in the game
    struct Ball {
        // init the ball's init position & velocity
        Ball(glm::vec2 pos_, glm::vec2 vel_):position(pos_), velocity(vel_), age(0) {
            trail.emplace_back(position, trail_length);
            trail.emplace_back(position, 0.0f);
        }
        glm::vec2 position;
        // velocity will increase as time goes by
        glm::vec2 velocity;
        std::deque< glm::vec3 > trail;
        // how long does this ball exist (to calculate the speed)
        float age;
    };
    std::vector<Ball> balls;

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "KillerPongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
