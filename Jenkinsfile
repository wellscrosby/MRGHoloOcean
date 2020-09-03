pipeline {
  agent { dockerfile true }
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
        sh '''
          py.test
        '''
      }
    }

  }
}
