/*
mock-up of multithreaded GLSL to SPIR-V shader compiler for vulkan renderer ~ Sunny 2025

pre-compiles .vert, .frag, .geom and .tmpl.

.tmpl:  assign a shader template and iteratate over a set define flags, 
        it auto-generates shader variantions data and a binding file.
        which is included in vk_shaders.cpp.
            - shaders/spirv/shader_data.c
            - shaders/spirv/shader_binding.c
*/

#include <windows.h>
#include <process.h>            // _beginthreadex
#include <string>               // std::string
#include <vector>
#include <cstdio>               // printf, fprintf
#include <cstdlib>              // system, getenv
#include <cstring>              // strcpy, strcat
#include <io.h>                 // _findfirst, _findnext
#include <direct.h>             // _mkdir
#include <iostream>

#define MAX_TASKS 1500
#define MAX_THREADS 16 

#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )

typedef struct {
    char            c_var_name[MAX_PATH];       // variable name
    char            c_binding_name[MAX_PATH];   // binding array structure
    int             size;                       // length/size of spirv bytes
    unsigned char   *data;                      // shader data
} spirvOut;

spirvOut output_shaders[MAX_TASKS];

typedef struct {
    int     index;
    char    input_file[MAX_PATH];
    char    output_var_name[128];
    char    output_binding_name[128];
    char    spirv_out[MAX_PATH];
    char    stage[8];
    char    output_file[MAX_PATH];
    char    defines[256]; 
} ShaderTask;

typedef struct {
    std::string env_path;
    std::string base_path;
} compiler_data_t;

ShaderTask tasks[MAX_TASKS];
HANDLE threads[MAX_THREADS];

static uint32_t task_count          = 0;
static uint32_t thread_count        = 0;
static uint32_t bind_shaders_count  = 0;
static uint32_t failed_count        = 0;

static std::string glsl_root_path;
static std::string glsl_lang_validator;
static std::string out_data_path;
static std::string out_bindings_path;

static compiler_data_t compiler;

std::string join_flags( std::initializer_list<const char*> flags ) 
{
    std::string result;

    for ( auto f : flags ) 
    {
        if ( f && *f ) 
        {
            result += f;
            result += " ";
        }
    }

    return result;
}

std::string join_indexes( std::string object, std::initializer_list<uint32_t> flags ) 
{
    std::string result = object;
    for (int f : flags) {
        result += "[" + std::to_string(f) + "]";
    }
    return result;
}

static std::string create_compiler_cmd( ShaderTask* task, bool silent = true) 
{
    char cmd[1024];

    const char *silent_flag = silent ? "-s " : "";

    snprintf(cmd, sizeof(cmd), "\"%s\" %s -S %s -V -o %s %s %s",
        compiler.base_path.c_str(),
        silent_flag,
        task->stage,
        task->spirv_out,
        task->input_file,
        task->defines
    );

    return std::string( cmd );
}

unsigned __stdcall compile_shader_thread( void* arg ) 
{
    ShaderTask* task = (ShaderTask*)arg;

    printf("%s = %s\n", task->output_binding_name, task->output_var_name );
    //printf("cmd: %s\n\n", cmd, task->output_array_name );

    // https://github.com/KhronosGroup/glslang/issues/1059 - like to use stdout ..
    int result = system( create_compiler_cmd( task ).c_str() );
    if (result != 0) {
        std::string cmd = create_compiler_cmd( task, false );

        fprintf(stderr, "[Thread] Failed to compile %s: %s\n", task->input_file, task->output_var_name);
        fprintf(stderr, "cmd: %s\n\n", cmd.c_str() );
        // re-run the command, not silent to get compiler error
        system( cmd.c_str() );

        failed_count++;
        return 1;
    }

    // load SPIR-V file to memory
    FILE *f_in = fopen(task->spirv_out, "rb");
    if (!f_in) {
        fprintf(stderr, "Could not open SPIR-V file: %s\n", task->spirv_out);
        return 1;
    }

    fseek(f_in, 0, SEEK_END);
    long file_size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    unsigned char *shader_data = (unsigned char*)malloc(file_size);
    if (!shader_data) {
        fprintf(stderr, "Failed to allocate memory for shader data.\n");
        fclose(f_in);
        return 1;
    }

    fread(shader_data, 1, file_size, f_in);
    fclose(f_in);

    // store shader data to output list
    spirvOut *output = &output_shaders[task->index];
    output->size = file_size;
    output->data = shader_data;
    snprintf(output->c_var_name, MAX_PATH, "%s", task->output_var_name);
    snprintf(output->c_binding_name, sizeof(output->c_binding_name), "%s", task->output_binding_name);

    // remove temp SPIR-V file
    remove(task->spirv_out);

    return 0;
}

static void create_shader_task( const char *f_name, const char *stage, const char *out_var, const char *out_binding, const char *defines ) 
{
    // wait for thread queue before adding new task if thread pool is overflowing
    if ( thread_count >= MAX_THREADS ) {
        WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);

        for ( int j = 0; j < thread_count; ++j )
            CloseHandle(threads[j]);

        thread_count = 0; // reset thread count after processing
    }

    ShaderTask *t = &tasks[task_count];
    t->index = task_count;

    snprintf(t->input_file, MAX_PATH, "%s\\%s", glsl_root_path.c_str(), f_name);
    snprintf(t->spirv_out, MAX_PATH, "spirv\\tmp_%d.spv", task_count + 1);
    snprintf(t->stage, 8, "%s", stage);
    snprintf(t->output_var_name, MAX_PATH, "%s", out_var);

    if ( out_binding ) 
    {
        snprintf(t->output_binding_name, MAX_PATH, "%s", out_binding);
        bind_shaders_count++;
    }
    else
        t->output_binding_name[0] = '\0';

    snprintf(t->defines, sizeof(t->defines), "%s", defines);

    threads[thread_count++] = (HANDLE)_beginthreadex(NULL, 0, compile_shader_thread, t, 0, NULL);
    task_count++;
}

static void compile_and_convert_template_shaders( void )
{
    uint32_t i, j, k, l, m, n, o;
    std::string defines, name, ids, defines_cl, name_cl, ids_cl;

    const char* mode_flags[]       = { "-DUSE_CLX_IDENT", "-DUSE_FIXED_COLOR", };
    const char* mode_ids[]         = { "ident1", "fixed" };

    const char* vbo_flags[]         = { "", "-DUSE_VBO_GHOUL2", "-DUSE_VBO_MDV" };
    const char* vbo_ids[]           = { "", "ghoul2_", "mdv_" };

    const char* tx_flags[]          = { "", "-DUSE_TX1", "-DUSE_TX2" };
    const char* tx_ids[]            = { "tx0", "tx1", "tx2" };

    const char* cl_flags[]          = { "", "-DUSE_CL1", "-DUSE_CL2" };
    const char* cl_ids[]            = { "", "cl", "cl" };

    const char* env_flags[]         = { "", "-DUSE_ENV" };
    const char* env_ids[]           = { "", "_env" };

    const char* fog_flags[]         = { "", "-DUSE_FOG" };
    const char* fog_ids[]           = { "", "_fog" };

    // fog only
    const char* fog_only_flags[]    = { "-DUSE_FOG_LINEAR", "-DUSE_FOG_EXP" };
    const char* fog_only_ids[]      = { "linear", "exp" };

    // single-texture fragment, depth-fragment
    create_shader_task("gen_frag.tmpl", "frag", "frag_tx0_df", NULL, "-DUSE_CLX_IDENT -DUSE_ATEST -DUSE_DF");

    //  compile lighting shader variations from templates
    create_shader_task( "light_vert.tmpl", "vert", "vert_light", NULL, "");
    create_shader_task( "light_vert.tmpl", "vert", "vert_light_fog", NULL, "-DUSE_FOG");
                        
    create_shader_task( "light_frag.tmpl", "frag", "frag_light", NULL, "");
    create_shader_task( "light_frag.tmpl", "frag", "frag_light_fog", NULL, "-DUSE_FOG");
                        
    create_shader_task( "light_frag.tmpl", "frag", "frag_light_line", NULL, "-DUSE_LINE");
    create_shader_task( "light_frag.tmpl", "frag", "frag_light_line_fog", NULL, "-DUSE_LINE -DUSE_FOG");

    // surface sprites
    create_shader_task( "surface_sprite_vert.tmpl", "vert", "vert_surface_sprites", NULL, "");
    create_shader_task( "surface_sprite_vert.tmpl", "vert", "vert_surface_sprites_fog", NULL, "-DUSE_FOG");
    
    create_shader_task( "surface_sprite_frag.tmpl", "frag", "frag_surface_sprites", NULL, "-DUSE_ATEST");
    create_shader_task( "surface_sprite_frag.tmpl", "frag", "frag_surface_sprites_fog", NULL, "-DUSE_ATEST -DUSE_FOG");

    std::string underscore = "";

    // fog-only shaders
    for ( i = 0; i < ARRAY_LEN(vbo_flags); ++i ) { // vbo
        for ( j = 0; j < ARRAY_LEN(fog_only_flags); ++j ) { // fog only mode
            defines = join_flags({ vbo_flags[i], fog_only_flags[j] });
            name    = "vert_fog_only_" + std::string(vbo_ids[i]) + fog_only_ids[j];
            ids     = join_indexes("vk.shaders.vert.fog", { i, j });

            create_shader_task("fog_vert.tmpl", "vert", name.c_str(), ids.c_str(), defines.c_str()); 
        }
    }

    for ( i = 0; i < ARRAY_LEN(fog_only_flags); ++i ) { // vbo
        defines = join_flags({ fog_only_flags[i] });
        name    = "frag_fog_only_" + std::string(fog_only_ids[i]);
        ids     = join_indexes("vk.shaders.frag.fog", { i });
        create_shader_task("fog_frag.tmpl", "frag", name.c_str(), ids.c_str(), defines.c_str());
    }

    // ident / fixed vertex shaders
    for ( i = 0; i < ARRAY_LEN(vbo_flags); ++i ) { // vbo
        for ( j = 0; j < (ARRAY_LEN(tx_flags) - 1); ++j ) { // tx [0,1 only]
            for ( m = 0; m < ARRAY_LEN(mode_flags); ++m ) { // mode (ident / fixed)
                for ( k = 0; k < ARRAY_LEN(env_flags); ++k ) { // env
                    for ( l = 0; l < ARRAY_LEN(fog_flags); ++l ) { // fog
                        defines = join_flags({ vbo_flags[i], tx_flags[j], mode_flags[m], env_flags[k], fog_flags[l] });
                        name    = "vert_" + std::string(vbo_ids[i]) + tx_ids[j] + "_" + mode_ids[m] + env_ids[k] + fog_ids[l];
                        ids     = join_indexes("vk.shaders.vert." + std::string(mode_ids[m]), { i, j, k, l });

                        create_shader_task("gen_vert.tmpl", "vert", name.c_str(), ids.c_str(), defines.c_str());
                    }
                }
            }
        }
    }

    // ident / fixed fragment shaders
    for ( i = 0; i < ARRAY_LEN(vbo_flags); ++i ) { // vbo
        for ( j = 0; j < (ARRAY_LEN(tx_flags) - 1); ++j) { // tx [0,1 only]
            for ( m = 0; m < ARRAY_LEN(mode_flags); ++m ) { // mode (ident / fixed)
                for ( k = 0; k < ARRAY_LEN(fog_flags); ++k ) { // fog
                    defines = join_flags({ vbo_flags[i], tx_flags[j], mode_flags[m], fog_flags[k] });

                    if ( j == 0 ) 
                        defines += " -DUSE_ATEST";

                    name    = "frag_" + std::string(vbo_ids[i]) + tx_ids[j] + "_" + mode_ids[m] + fog_ids[k];
                    ids     = join_indexes("vk.shaders.frag." + std::string(mode_ids[m]), { i, j, k });

                    create_shader_task("gen_frag.tmpl", "frag", name.c_str(), ids.c_str(), defines.c_str());
                }
            }
        }
    }



    // generic vertex shaders
    for ( i = 0; i < ARRAY_LEN(vbo_flags); ++i ) { // vbo
        defines = join_flags({ vbo_flags[i] });
        name    = "refraction_" + std::string(vbo_ids[i]);
        ids     = join_indexes("vk.shaders.refraction_vs", { i });
        create_shader_task("refraction.tmpl", "vert", name.c_str(), ids.c_str(), defines.c_str());

        for ( j = 0; j < ARRAY_LEN(tx_flags); ++j ) { // tx
            for ( k = 0; k < ARRAY_LEN(env_flags); ++k ) { // env
                for ( l = 0; l < ARRAY_LEN(fog_flags); ++l ) { // fog
                    defines = join_flags({ vbo_flags[i], tx_flags[j], env_flags[k], fog_flags[l] });
                    name    = "vert_" + std::string(vbo_ids[i]) + tx_ids[j] + env_ids[k] + fog_ids[l];
                    ids     = join_indexes("vk.shaders.vert.gen", { i, j, 0, k, l });

                    create_shader_task("gen_vert.tmpl", "vert", name.c_str(), ids.c_str(), defines.c_str());

                    if ( j != 0 ) // tx
                    { 
                        defines_cl = join_flags({ vbo_flags[i], tx_flags[j], cl_flags[j], env_flags[k], fog_flags[l] });
                        name_cl    = "vert_" + std::string(vbo_ids[i]) + tx_ids[j] + "_" + cl_ids[j] + env_ids[k] + fog_ids[l];
                        ids_cl     = join_indexes("vk.shaders.vert.gen", { i, j, 1, k, l });

                        create_shader_task("gen_vert.tmpl", "vert", name_cl.c_str(), ids_cl.c_str(), defines_cl.c_str());
                    }
                }
            }
        }
    }

    // generic fragment shaders
    for ( i = 0; i < ARRAY_LEN(vbo_flags); ++i ) { // vbo
        for ( j = 0; j < ARRAY_LEN(tx_flags); ++j) { // tx
            for ( k = 0; k < ARRAY_LEN(fog_flags); ++k ) { // fog
                defines = join_flags({ vbo_flags[i], tx_flags[j], fog_flags[k] });

                if ( j == 0 ) 
                    defines += " -DUSE_ATEST";

                name    = "frag_" + std::string(vbo_ids[i]) + tx_ids[j] + fog_ids[k];
                ids     = join_indexes("vk.shaders.frag.gen", { i, j, 0, k });

                create_shader_task("gen_frag.tmpl", "frag", name.c_str(), ids.c_str(), defines.c_str());

                if ( j != 0 ) // tx
                { 
                    defines_cl  = join_flags({ vbo_flags[i], tx_flags[j], cl_flags[j], fog_flags[k] });
                    name_cl     = "frag_" + std::string(vbo_ids[i])  + tx_ids[j] + "_" + cl_ids[j] + fog_ids[k];
                    ids_cl      = join_indexes("vk.shaders.frag.gen", { i, j, 1, k });

                    create_shader_task("gen_frag.tmpl", "frag", name_cl.c_str(), ids_cl.c_str(), defines_cl.c_str());
                }
            }
        }
    }
}

static void compile_and_convert_individual_shaders( void )
{
    const char *stages[] = { "vert", "frag", "geom" };
    const char *stage_exts[] = { ".vert", ".frag", ".geom" };

    char find_pattern[256];
    struct _finddata_t f;
    intptr_t handle;

    char input_file[256], cmd[512], array_name[128];

    for (int i = 0; i < 3; ++i) {
        snprintf(find_pattern, sizeof(find_pattern), "%s\\*%s", glsl_root_path.c_str(), stage_exts[i]);
        handle = _findfirst(find_pattern, &f);

        if (handle == -1) continue;

        do {
            char base_name[256];
            strncpy(base_name, f.name, sizeof(base_name));
            base_name[sizeof(base_name) - 1] = '\0';

            // remove stage extension from base_name
            char *ext = strstr(base_name, stage_exts[i]);
            if (ext) *ext = '\0'; // truncate the string at the extension

            char out_var[256];
            snprintf(out_var, sizeof(out_var), "%s_%s_spv", base_name, stages[i]);
            create_shader_task( f.name, stages[i], out_var, NULL, "" );

        } while (_findnext(handle, &f) == 0);

        _findclose(handle);
    }
}

static void compile_and_convert_shaders( void ) 
{
    compile_and_convert_individual_shaders();
    compile_and_convert_template_shaders();

    // wait for remaining threads to finish if any
    if (thread_count > 0) 
    {
        uint32_t i;
        WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);

        for ( i = 0; i < thread_count; ++i )
            CloseHandle(threads[i]);

        thread_count = 0;
    }
}

static void write_all_shaders_to_file( const char *out_file ) 
{
    uint32_t i, j;
    spirvOut *output;
    FILE *f_out;

    f_out = fopen(out_file, "wb");
    
    if ( !f_out ) {
        fprintf( stderr, "Could not open output file for writing: %s\n", out_file );
        return;
    }

    const int line_length = 16;

    for ( i = 0; i < task_count; ++i )
    {
        output = &output_shaders[i];

        if ( output->size == 0 )
            continue;

        fprintf( f_out, "const unsigned char %s[%d] = {\n\t", output->c_var_name, output->size );

        unsigned char *data = output->data;

        for ( j = 0; j < output->size; ++j )
        {
            fprintf( f_out, "0x%.2X", data[j] );

            if ( j + 1 < output->size )
            {
			    if ( (j + 1) % line_length )
				    fputs( ", ", f_out );
			    else
				    fputs( ",\n\t", f_out );
            }
        }

        fputs( "\n};\n", f_out ); 
        free( output->data );
    }

    fclose( f_out );

    printf( "written %i shaders to %s\n\n", i, out_file );
}

static void write_all_shaders_bindings_to_file( const char *out_file ) 
{
    uint32_t i;
    spirvOut *output;
    FILE *f_out;

    f_out = fopen(out_file, "wb");
    
    if ( !f_out ) {
        fprintf(stderr, "Could not open output file for writing: %s\n", out_file);
        return;
    }

    fprintf( f_out, "// this file is autogenerated during shader compilation\n" );
    fprintf( f_out, "static void vk_set_shader_name( VkShaderModule shader, const char *name ) {\n" );
    fprintf( f_out, "    VK_SET_OBJECT_NAME( shader, name, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );\n" );
    fprintf( f_out, "}\n" );
    fprintf( f_out, "void vk_bind_generated_shaders( void ){\n" );

    uint32_t bound_shaders_count = 0;

    for ( i = 0; i < task_count; ++i ) 
    {
        output = &output_shaders[i];

        if ( output->size == 0 )
            continue;

        if ( output->c_binding_name[0] == '\0' )
            continue;

        fprintf( f_out, "    %s = SHADER_MODULE( %s );\n", output->c_binding_name, output->c_var_name );
        fprintf( f_out, "    vk_set_shader_name( %s, \"%s\" );\n", output->c_binding_name, output->c_var_name );\

        bound_shaders_count++;
    }

    fprintf( f_out, "}" );
    fclose( f_out );

    printf( "written %i of %i bindings to %s\n", bound_shaders_count, bind_shaders_count, out_file );
}

static void set_glsl_spirv_compiler_env( std::string compiler_path ) 
{
    memset( &compiler, 0, sizeof(compiler_data_t) );

    if ( compiler_path.empty() ) {
        printf("Error: compiler path is empty\n");
        return;
    }

    compiler.base_path = compiler_path; // use input argument as base path when no environment is found

    size_t start = compiler_path.find('%');
    size_t end = compiler_path.find('%', start + 1);

    if ( start != std::string::npos && end != std::string::npos && start < end ) 
    {
        std::string var_name = compiler_path.substr(start + 1, end - start - 1);
        const char* env_value = std::getenv(var_name.c_str());

        if ( !env_value ) {
            std::cerr << "Environment variable " << var_name << " is not set! \n";
            return;
        }

        compiler.base_path = std::string(env_value) + compiler_path.substr(end + 1);
    }   
}

int main( int argc, const char* argv[] )
{
    if (argc < 4) {
        std::cerr << "Usage: <exe> <glsl_path> <validator_path> <out_data> <out_bindings>\n";
        return -1;
    }

	if ( strlen( argv[1] ) > sizeof( glsl_root_path ) ) {
		printf( "glsl path name is too long %s\n", argv[1] );
		return -1;
	}

    // perhaps some more sanity checks ..

    glsl_root_path       = argv[1];
    glsl_lang_validator  = argv[2];
    out_data_path        = argv[3];
    out_bindings_path    = argv[4];

    out_data_path = out_data_path.substr(1);
    out_bindings_path = out_bindings_path.substr(1);

    set_glsl_spirv_compiler_env( glsl_lang_validator);

    std::cout << "glsl path:" << glsl_root_path << "\n";
    std::cout << "glsl compiler:" << compiler.base_path << "\n";
    std::cout << "out data:" << out_data_path << "\n";
    std::cout << "out bindings:" << out_bindings_path << "\n";

    _mkdir("spirv");
    remove(out_data_path.c_str());
    remove(out_bindings_path.c_str());

    compile_and_convert_shaders();

    printf("\nresult:\n");
    write_all_shaders_bindings_to_file( out_bindings_path.c_str() );
    write_all_shaders_to_file( out_data_path.c_str() );

    printf("succeeded: %i of %i\n\n", task_count - failed_count, task_count );

    return 0;
}
