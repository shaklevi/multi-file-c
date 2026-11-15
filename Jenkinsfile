// Declarative pipeline for fibtool.
// Needs on the agent: gcc, make, git, tar. Optional: cppcheck (Lint stage).
// Plugins used: JUnit (bundled), Timestamper (optional but nice).
pipeline {
    // To build in a container instead, replace with:
    //   agent { docker { image 'gcc:13' } }
    agent any

    options {
        timestamps()
        timeout(time: 15, unit: 'MINUTES')
        buildDiscarder(logRotator(numToKeepStr: '20'))
        disableConcurrentBuilds()
    }

    environment {
        WERROR = '1'                 // CI is strict: warnings fail the build
        // BUILD_NUMBER is injected by Jenkins and picked up by the Makefile
    }

    stages {
        stage('Toolchain info') {
            steps {
                sh 'gcc --version | head -1 && make --version | head -1'
            }
        }

        stage('Build + Test') {
            // Each flavour has its own artifacts/<flavour>/ dir, so these are safe in parallel
            parallel {
                stage('release') {
                    steps { sh 'make test BUILD=release' }
                    post { always { junit allowEmptyResults: false, testResults: 'artifacts/release/test-reports/*.xml' } }
                }
                stage('debug') {
                    steps { sh 'make test BUILD=debug' }
                    post { always { junit 'artifacts/debug/test-reports/*.xml' } }
                }
                stage('sanitizers') {
                    steps { sh 'make test BUILD=asan' }
                    post { always { junit 'artifacts/asan/test-reports/*.xml' } }
                }
            }
        }

        stage('Lint') {
            steps {
                // Don't fail the whole build if cppcheck is missing - mark it UNSTABLE instead
                catchError(buildResult: 'SUCCESS', stageResult: 'UNSTABLE') {
                    sh 'make lint'
                }
            }
        }

        stage('Package') {
            // Only package for main / release branches, not for every PR
            when { anyOf { branch 'main'; branch 'master'; buildingTag() } }
            steps {
                sh 'make dist BUILD=release'
                archiveArtifacts artifacts: 'artifacts/*.tar.gz', fingerprint: true
            }
        }
    }

    post {
        success { echo "Build ${env.BUILD_NUMBER} OK" }
        failure { echo "Build ${env.BUILD_NUMBER} FAILED" }
        cleanup { sh 'make distclean || true' }
    }
}
