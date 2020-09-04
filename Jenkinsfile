pipeline {
  agent {
    dockerfile {
      args '--runtime=nvidia'
    }

  }
  stages {
    stage('build') {
      steps {
        sh '''
          pip install .
        '''
      }
    }

    stage('test') {
      steps {
        sh 'pytest'
      }
    }

  }
}