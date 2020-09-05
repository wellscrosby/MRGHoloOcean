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
