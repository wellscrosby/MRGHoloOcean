pipeline {
  agent { python:slim }
  stages {
    stage('build') {
      steps {
        sh '''
          pip install pytest opencv-python
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
