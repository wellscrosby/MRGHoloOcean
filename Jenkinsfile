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
        sh 'ls'
        sh 'pwd'
        sh 'runuser -l holodeckuser -c \\"cd /var/jenkins_home/jobs/holodeck-ocean/branches/jenkins/workspace && pytest\\"'
      }
    }

  }
}