# --------------------------------------------------------------------------
#                   OpenMS -- Open-Source Mass Spectrometry
# --------------------------------------------------------------------------
# Copyright OpenMS Inc. -- Eberhard Karls University Tuebingen,
# ETH Zurich, and Freie Universitaet Berlin 2002-present.
#
# This software is released under a three-clause BSD license:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of any author or any participating institution
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# For a full list of authors, refer to the file AUTHORS.
# --------------------------------------------------------------------------
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
# INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# --------------------------------------------------------------------------
# $Maintainer: OpenMS Team $
# $Authors: OpenMS Team $
# --------------------------------------------------------------------------

# Fetch cpp-httplib library for HTTP requests
include(FetchContent)

FetchContent_Declare(httplib 
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.26.0
)

FetchContent_MakeAvailable(httplib)

# cpp-httplib is header-only, create an interface library if target doesn't exist
if(NOT TARGET httplib::httplib)
    add_library(httplib::httplib INTERFACE)
    target_include_directories(httplib::httplib INTERFACE ${httplib_SOURCE_DIR})
endif()

# Add license to third party licenses file
if(EXISTS "${httplib_SOURCE_DIR}/LICENSE")
    cat("${httplib_SOURCE_DIR}/LICENSE" ${TPLICENSEFILE})
endif()