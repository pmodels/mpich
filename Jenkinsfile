pipeline {
    agent none

    stages {
        stage('Lint') {
            stages {
                stage('RPM Lint') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.centos.7'
                            label 'docker_runner'
                            additionalBuildArgs  '--build-arg UID=$(id -u)'
                            args  '--group-add mock --cap-add=SYS_ADMIN --privileged=true'
                        }
                    }
                    steps {
                        sh 'make -f Makefile-rpm.mk rpmlint'
                    }
                }
            }
        }
        stage('Build') {
            parallel {
                stage('Build on CentOS 7') {
                    agent {
                        dockerfile {
                            filename 'Dockerfile.centos.7'
                            label 'docker_runner'
                            additionalBuildArgs '--build-arg UID=$(id -u) --build-arg JENKINS_URL=' +
                                                env.JENKINS_URL
                            args  '--group-add mock --cap-add=SYS_ADMIN --privileged=true'
                        }
                    }
                    steps {
                        sh '''rm -rf artifacts/centos7/
                              mkdir -p artifacts/centos7/
                              make -f Makefile-rpm.mk srpm
                              make -f Makefile-rpm.mk mockbuild'''
                    }
                    post {
                        success {
                            sh '''(cd /var/lib/mock/epel-7-x86_64/result/ &&
                                   cp -r . $OLDPWD/artifacts/centos7/)
                                  createrepo artifacts/centos7/'''
                            archiveArtifacts artifacts: 'artifacts/centos7/**'
                        }
                        failure {
                            sh '''cp -af _topdir/SRPMS artifacts/centos7/
                                  (cd /var/lib/mock/epel-7-x86_64/result/ &&
                                   cp -r . $OLDPWD/artifacts/centos7/)
                                  (cd /var/lib/mock/epel-7-x86_64/root/builddir/build/BUILD/*/
                                   find . -name configure -printf %h\\\\n | \
                                   while read dir; do
                                       if [ ! -f $dir/config.log ]; then
                                           continue
                                       fi
                                       tdir="$OLDPWD/artifacts/centos7/autoconf-logs/$dir"
                                       mkdir -p $tdir
                                       cp -a $dir/config.log $tdir/
                                   done)'''
                            archiveArtifacts artifacts: 'artifacts/centos7/**'
                        }
                    }
                }
                stage('Build on Ubuntu 16.04') {
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
