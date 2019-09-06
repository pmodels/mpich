#!/usr/bin/groovy
/* Copyright (C) 2019 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the
 *    documentation and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 *  4. All publications or advertising materials mentioning features or use of
 *     this software are asked, but not required, to acknowledge that it was
 *     developed by Intel Corporation and credit the contributors.
 *
 * 5. Neither the name of Intel Corporation, nor the name of any Contributor
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// To use a test branch (i.e. PR) until it lands to master
// I.e. for testing library changes
//@Library(value="pipeline-lib@your_branch") _

pipeline {
    agent none

    stages {
        stage('Cancel Previous Builds') {
            when { changeRequest() }
            steps {
                cancelPreviousBuilds()
            }
        }
        stage('Lint') {
            parallel {
                stage('RPM Lint') {
                    agent {
                        dockerfile {
                            filename 'packaging/Dockerfile.centos.7'
                            label 'docker_runner'
                            args  '--group-add mock' +
                                  ' --cap-add=SYS_ADMIN' +
                                  ' --privileged=true'
                            additionalBuildArgs  '--build-arg UID=$(id -u)'
                        }
                    }
                    steps {
                        sh 'make -f Makefile-rpm.mk rpmlint'
                    }
                }
                stage('Check Packaging') {
                    agent { label 'lightweight' }
                    steps {
                        checkoutScm url: 'https://github.com/daos-stack/packaging.git',
                                    checkoutDir: 'packaging-module'
                        catchError(stageResult: 'UNSTABLE', buildResult: 'SUCCESS') {
                            sh '''make -f Makefile-rpm.mk                      \
                                          PACKAGING_CHECK_DIR=packaging-module \
                                          packaging_check'''
                        }
                    }
                    post {
                        unsuccessful {
                            emailext body: 'Packaging out of date for ' + jobName() + '.\n' +
                                           "You should update it and submit your PR again.",
                                     recipientProviders: [
                                          [$class: 'DevelopersRecipientProvider'],
                                          [$class: 'RequesterRecipientProvider']
                                     ],
                                     subject: 'Packaging is out of date for ' + jobName()

                        }
                    }
                } //stage('Check Packaging')
            } // parallel
        } //stage('Lint')
        stage('Build') {
            parallel {
                stage('Build on CentOS 7') {
                    agent {
                        dockerfile {
                            filename 'packaging/Dockerfile.centos.7'
                            label 'docker_runner'
                            args  '--group-add mock' +
                                  ' --cap-add=SYS_ADMIN' +
                                  ' --privileged=true'
                            additionalBuildArgs '--build-arg UID=$(id -u)' +
                                                ' --build-arg JENKINS_URL=' +
                                                env.JENKINS_URL
                        }
                    }
                    steps {
                        sh label: "Build package",
                        script: '''rm -rf artifacts/centos7/
                              mkdir -p artifacts/centos7/
                              make -f Makefile-rpm.mk chrootbuild'''
                    }
                    post {
                        success {
                             sh label: "Collect artifacts",
                            script: '''(cd /var/lib/mock/epel-7-x86_64/result/ &&
                                   cp -r . $OLDPWD/artifacts/centos7/)
                                  createrepo artifacts/centos7/'''
                        }
                        unsuccessful {
                            sh label: "Collect artifacts",
                               script: '''mockroot=/var/lib/mock/epel-7-x86_64
                                  artdir=$PWD/artifacts/centos7
                                  cp -af _topdir/SRPMS $artdir
                                  (cd $mockroot/result/ &&
                                   cp -r . $artdir)
                                  (if cd $mockroot/root/builddir/build/BUILD/*/; then
                                   find . -name configure -printf %h\\\\n | \
                                   while read dir; do
                                       if [ ! -f $dir/config.log ]; then
                                           continue
                                       fi
                                       tdir="$artdir/autoconf-logs/$dir"
                                       mkdir -p $tdir
                                       cp -a $dir/config.log $tdir/
                                   done
                              fi)'''
                        }
                        cleanup {
                            archiveArtifacts artifacts: 'artifacts/centos7/**'
                        }
                    }
                } //stage('Build on CentOS 7')
                stage('Build on Ubuntu 18.04') {
                    agent {
                        label 'docker_runner'
                    }
                    steps {
                        echo "Building on Ubuntu is not implemented for the moment"
                    }
                }
            }
        }
    }
}
