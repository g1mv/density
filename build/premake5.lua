#!lua

--
-- Centaurean Density
--
-- Copyright (c) 2015, Guillaume Voirin
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions are met:
--
--     1. Redistributions of source code must retain the above copyright notice, this
--        list of conditions and the following disclaimer.
--
--     2. Redistributions in binary form must reproduce the above copyright notice,
--        this list of conditions and the following disclaimer in the documentation
--        and/or other materials provided with the distribution.
--
--     3. Neither the name of the copyright holder nor the names of its
--        contributors may be used to endorse or promote products derived from
--        this software without specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-- DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
-- DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
-- SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
-- CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
-- OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--
-- 1/05/15 23:52
--

-- Check for tools
if os.execute("clang -v") == 0 then
	toolset "clang"
elseif os.execute("gcc -v") == 0 then
	toolset "gcc"
else
	io.write("No supported compiler found on the command line. Please install Clang/LLVM or GCC.\n")
	os.exit(0)
end

if os.execute("git --version") > 0 then
	io.write("Please install Git, it is required for submodules updating.")
	os.exit(0)
end

-- Submodules update
os.execute("git submodule update --init --recursive")

solution "Density"
	configurations { "Release" }
	buildoptions { "-std=c99" }
	flags { "OptimizeSpeed", "NoFramePointer", "LinkTimeOptimization" }

	project "spookyhash"
		kind "StaticLib"
		language "C"
		files {
			"../src/spookyhash/src/*.h",
			"../src/spookyhash/src/*.c"
		}

	project "density"
		kind "StaticLib"
		language "C"
		files {
			"../src/*.h",
			"../src/*.c"
		}
		links { "spookyhash" }

	project "benchmark"
		kind "ConsoleApp"
		language "C"
		files {
			"../benchmark/src/**.h",
			"../benchmark/src/**.c"
		}
		links { "density", "spookyhash" }
