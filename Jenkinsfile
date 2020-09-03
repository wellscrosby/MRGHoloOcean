pipeline {
  agent { 
      docker{ image 'python' }
  }
  stages {
    stage('build') {
      steps {
        sh '''
          apt install libgl1-mesa-glx
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
