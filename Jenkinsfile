pipeline {
  agent {
    dockerfile {
      args '--runtime=nvidia'
    }
  }

  stages {
    stage('Build') {
      steps {
        sh 'pip install .'
        sh 'python3 -c "import holodeck; holodeck.install(\\"Ocean\\", \\"https://robots.et.byu.edu/jenkins/job/holodeck-ocean-engine/job/optical_modem/lastSuccessfulBuild/artifact/Ocean.zip\\")"'
      }
    }

    stage('Test') {
      steps {
        sh 'py.test --junitxml results.xml'
      }
    }
  }
  post {
        always {
            junit 'results.xml'
        }
    }
}
