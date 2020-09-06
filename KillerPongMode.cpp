#include "KillerPongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <math.h>
#include <limits>

KillerPongMode::KillerPongMode() {
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of KillerPongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

KillerPongMode::~KillerPongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool KillerPongMode::is_complete() {
    return left_hp <= 0 || right_hp <= 0;
}

bool KillerPongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_MOUSEMOTION && !is_complete()) {
	    // handle mouse only when the game did not end
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);
		left_paddle.y = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).y;
	}

	return false;
}

void KillerPongMode::update(float elapsed) {
    if(is_complete()) {
        // either player does not have a hp left. do not update the game
        return;
    }

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- paddle update -----

	{//right player ai
	    // logic: find the nearest in-coming ball, move away from this ball
	    // traverse all balls to find the nearest in-coming one
	    Ball* ball_p = nullptr;
	    float min_distance = std::numeric_limits<float>::max();
	    for(auto& ball: balls) {
            float off_x = ball.position.x - right_paddle.x;
            float off_y = ball.position.y - right_paddle.y;
	        float distance = off_x * off_x + off_y * off_y;
            if(ball.velocity.x > 0 && distance < min_distance) {
                ball_p = &ball;
                min_distance = distance;
            }
	    }

	    if(ball_p) {
	        float hit_position_y = ball_p->position.y + (right_paddle.x - (ball_p->position).x) * (ball_p->velocity).y / (ball_p->velocity).x;
	        if (hit_position_y > right_paddle.y - paddle_radius.y - ball_radius.y * 8 &&
	            hit_position_y < right_paddle.y + paddle_radius.y + ball_radius.y * 8) {
	            // will be hit by the ball
	            if(ai_moving_direction != 0) {
	                // keep moving in original direction
	                right_paddle.y += ai_moving_direction * elapsed * ai_speed_factor;
                    if(right_paddle.y + paddle_radius.y + 0.1 >= court_radius.y) {
                        // hit upper wall, can only moving down
                        ai_moving_direction = -1;
                    } else if(right_paddle.y - paddle_radius.y - 0.1 <= - court_radius.y) {
                        // hit lower wall, can only moving up
                        ai_moving_direction = 1;
                    }
	            } else {
                    // starts to randomly move up or down
                    ai_moving_direction = mt() / float(mt.max()) > 0.5f ? 1 : -1;
	            }
	        } else {
	            // clear moving flag
	            ai_moving_direction = 0;
	        }
	    }
	}

	//clamp paddles to court:
	right_paddle.y = std::max(right_paddle.y, -court_radius.y + paddle_radius.y);
	right_paddle.y = std::min(right_paddle.y,  court_radius.y - paddle_radius.y);

	left_paddle.y = std::max(left_paddle.y, -court_radius.y + paddle_radius.y);
	left_paddle.y = std::min(left_paddle.y,  court_radius.y - paddle_radius.y);

	//----- ball update -----
	// update age & position of each ball
	for (Ball& ball : balls) {
        ball.age += elapsed;
        float speed_multiplier = std::pow((1 + ball_speed_inc_ratio),  (ball.age / ball_speed_inc_interval));
        ball.position += elapsed * ball.velocity * glm::min(ball_speed_max_multiplier, speed_multiplier);
	}

	// the last ball is the latest ball, check its age to see if we need to create a new one
	if (balls.size() == 0 || balls.back().age >= ball_create_interval) {
        float init_vel_x = (mt() / float(mt.max())) * 2 - 1; // [-1, 1]
        float init_vel_y = (mt() / float(mt.max())) * 2 - 1; // [-1, 1]
        // make sure x^2 + y^2 = 1
        init_vel_x = std::sqrt(init_vel_x * init_vel_x / (init_vel_x * init_vel_x + init_vel_y * init_vel_y)) * (init_vel_x > 0 ? 1 : -1);
        init_vel_y = std::sqrt(init_vel_y * init_vel_y / (init_vel_x * init_vel_x + init_vel_y * init_vel_y)) * (init_vel_y > 0 ? 1 : -1);
        balls.push_back(Ball(glm::vec2(0.0f, 0.0f), glm::vec2(init_vel_x, init_vel_y)));
	}

	//---- collision handling ----

	//paddles:
    auto paddle_vs_ball = [this](glm::vec2 const &paddle, Ball& ball, uint32_t &hp, float &invisible_elapsed) {
        //compute area of overlap:
        glm::vec2 min = glm::max(paddle - paddle_radius, ball.position - ball_radius);
        glm::vec2 max = glm::min(paddle + paddle_radius, ball.position + ball_radius);

        //if no overlap, no collision:
        if (min.x > max.x || min.y > max.y) {
            return;
        }

        if(invisible_elapsed >= invincible_sec) {
            // this paddle hit by a ball and not in invisible time, reduce 1 hp
            hp--;
            invisible_elapsed = 0;
        }

        if (max.x - min.x > max.y - min.y) {
            //wider overlap in x => bounce in y direction:
            if (ball.position.y > paddle.y) {
                ball.position.y = paddle.y + paddle_radius.y + ball_radius.y;
                ball.velocity.y = std::abs(ball.velocity.y);
            } else {
                ball.position.y = paddle.y - paddle_radius.y - ball_radius.y;
                ball.velocity.y = -std::abs(ball.velocity.y);
            }
        } else {
            //wider overlap in y => bounce in x direction:
            if (ball.position.x > paddle.x) {
                ball.position.x = paddle.x + paddle_radius.x + ball_radius.x;
                ball.velocity.x = std::abs(ball.velocity.x);
            } else {
                ball.position.x = paddle.x - paddle_radius.x - ball_radius.x;
                ball.velocity.x = -std::abs(ball.velocity.x);
            }
            //warp y velocity based on offset from paddle center:
            float vel = (ball.position.y - paddle.y) / (paddle_radius.y + ball_radius.y);
            ball.velocity.y = glm::mix(ball.velocity.y, vel, 0.75f);
        }
    };

    left_invincible_elapsed += elapsed;
    right_invincible_elapsed += elapsed;
    for(auto& ball: balls) {
        paddle_vs_ball(left_paddle, ball, left_hp, left_invincible_elapsed);
        paddle_vs_ball(right_paddle, ball, right_hp, right_invincible_elapsed);
    }

	//court walls:
	for(auto& ball: balls) {
        if (ball.position.y > court_radius.y - ball_radius.y) {
            ball.position.y = court_radius.y - ball_radius.y;
            if (ball.velocity.y > 0.0f) {
                ball.velocity.y = -ball.velocity.y;
            }
        }
        if (ball.position.y < -court_radius.y + ball_radius.y) {
            ball.position.y = -court_radius.y + ball_radius.y;
            if (ball.velocity.y < 0.0f) {
                ball.velocity.y = -ball.velocity.y;
            }
        }

        if (ball.position.x > court_radius.x - ball_radius.x) {
            ball.position.x = court_radius.x - ball_radius.x;
            if (ball.velocity.x > 0.0f) {
                ball.velocity.x = -ball.velocity.x;
            }
        }
        if (ball.position.x < -court_radius.x + ball_radius.x) {
            ball.position.x = -court_radius.x + ball_radius.x;
            if (ball.velocity.x < 0.0f) {
                ball.velocity.x = -ball.velocity.x;
            }
        }
    }

	//----- rainbow trails -----
    for(auto& ball: balls) {
        //age up all locations in ball trail:
        for (auto &t : ball.trail) {
            t.z += elapsed;
        }
        //store fresh location at back of ball trail:
        ball.trail.emplace_back(ball.position, 0.0f);

        //trim any too-old locations from back of trail:
        //NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
        while (ball.trail.size() >= 2 && ball.trail[1].z > trail_length) {
            ball.trail.pop_front();
        }
    }
}

void KillerPongMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x171714ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xd1bb54ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0x604d29ff);
	const std::vector< glm::u8vec4 > rainbow_colors = {
		HEX_TO_U8VEC4(0x604d29ff), HEX_TO_U8VEC4(0x624f29fc), HEX_TO_U8VEC4(0x69542df2),
		HEX_TO_U8VEC4(0x6a552df1), HEX_TO_U8VEC4(0x6b562ef0), HEX_TO_U8VEC4(0x6b562ef0),
		HEX_TO_U8VEC4(0x6d572eed), HEX_TO_U8VEC4(0x6f592feb), HEX_TO_U8VEC4(0x725b31e7),
		HEX_TO_U8VEC4(0x745d31e3), HEX_TO_U8VEC4(0x755e32e0), HEX_TO_U8VEC4(0x765f33de),
		HEX_TO_U8VEC4(0x7a6234d8), HEX_TO_U8VEC4(0x826838ca), HEX_TO_U8VEC4(0x977840a4),
		HEX_TO_U8VEC4(0x96773fa5), HEX_TO_U8VEC4(0xa07f4493), HEX_TO_U8VEC4(0xa1814590),
		HEX_TO_U8VEC4(0x9e7e4496), HEX_TO_U8VEC4(0xa6844887), HEX_TO_U8VEC4(0xa9864884),
		HEX_TO_U8VEC4(0xad8a4a7c),
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius)+s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(left_paddle+s, paddle_radius, shadow_color);
	draw_rectangle(right_paddle+s, paddle_radius, shadow_color);
	for(auto & ball: balls) {
        draw_rectangle(ball.position + s, ball_radius, shadow_color);
	}

	//ball's trail:
	for (auto& ball: balls) {
        if (ball.trail.size() >= 2) {
            //start ti at second element so there is always something before it to interpolate from:
            std::deque< glm::vec3 >::iterator ti = ball.trail.begin() + 1;
            //draw trail from oldest-to-newest:
            for (uint32_t i = uint32_t(rainbow_colors.size())-1; i < rainbow_colors.size(); --i) {
                //time at which to draw the trail element:
                float t = (i + 1) / float(rainbow_colors.size()) * trail_length;
                //advance ti until 'just before' t:
                while (ti != ball.trail.end() && ti->z > t) ++ti;
                //if we ran out of tail, stop drawing:
                if (ti == ball.trail.end()) break;
                //interpolate between previous and current trail point to the correct time:
                glm::vec3 a = *(ti-1);
                glm::vec3 b = *(ti);
                glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);
                //draw:
                draw_rectangle(at, ball_radius, rainbow_colors[i]);
            }
        }
	}


	//solid objects:

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//paddles:
	draw_rectangle(left_paddle, paddle_radius, fg_color);
	draw_rectangle(right_paddle, paddle_radius, fg_color);
	

	//ball:
	for(auto& ball: balls) {
        draw_rectangle(ball.position, ball_radius, fg_color);
	}

	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < left_hp; ++i) {
		draw_rectangle(glm::vec2( -court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}
	for (uint32_t i = 0; i < right_hp; ++i) {
		draw_rectangle(glm::vec2( court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	}

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
