// Copyright (C) 2008-2010 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "Eigenfaces.h"

Eigenfaces::Eigenfaces()
{
  m_pixelsPerImage = 0;
  m_preallocatedImages = 0;
}

Eigenfaces::~Eigenfaces()
{
}

void Eigenfaces::reserve(size_t numImages)
{
  if (numImages <= 0)
    throw std::invalid_argument("Invalid argument 'numImages' in Eigenfaces::reserve method.");

  if (m_pixelsPerImage > 0)
    m_dataSet.resize(m_pixelsPerImage, m_dataSet.cols() + numImages);

  m_preallocatedImages += numImages;
}

void Eigenfaces::addImage(const Vector& faceImage)
{
  // If this is the first added image...
  if (m_pixelsPerImage == 0) {
    // Setup the quantity of pixels per image
    m_pixelsPerImage = faceImage.size();

    // Rows(pixels) x columns(images)
    if (m_preallocatedImages > 0)
      m_dataSet.resize(m_pixelsPerImage, m_preallocatedImages--);
    else
      m_dataSet.resize(m_pixelsPerImage, 1);

    // It is the first image in the data set
    m_dataSet.setCol(0, faceImage);
  }
  else if (m_preallocatedImages > 0) {
    m_dataSet.setCol(m_dataSet.cols() - m_preallocatedImages, faceImage);
    m_preallocatedImages--;
  }
  else {
    if (m_pixelsPerImage != faceImage.size())
      throw std::invalid_argument("Invalid face: you cannot use different face sizes in the same Eigenfaces instance.");

    // Add a column in the data set (each column is an image)
    m_dataSet.addCol(m_dataSet.cols(), faceImage);
  }
}

/// Returns the number of training images available to calculate
/// eigenfaces.
///
size_t Eigenfaces::getImageCount() const
{
  return (m_pixelsPerImage == 0) ? 0: m_dataSet.cols() - m_preallocatedImages;
}

/// Returns the number of pixels per image.
///
size_t Eigenfaces::getPixelsPerImage() const
{
  return m_pixelsPerImage;
}

size_t Eigenfaces::getEigenfaceComponents() const
{
  return m_eigenfaceComponents;
}

size_t Eigenfaces::getEigenvaluesCount() const
{
  return m_eigenvalues.size();
}

/// Calculate eivenvalues and sort them in descendant order.
///
bool Eigenfaces::calculateEigenvalues()
{
  // If there are reserved columns we just removed them so they do
  // not mess all the calculations
  if (m_preallocatedImages > 0)
    m_dataSet.resize(m_pixelsPerImage, m_dataSet.cols() - m_preallocatedImages);

  // Calculate the mean of all faces
  m_dataSet.meanCol(m_meanFace);

  // Substract the mean from all faces
  m_dataSetZeroMean.resize(m_dataSet.rows(),
			   m_dataSet.cols());
  for (int j=0; j<m_dataSet.cols(); ++j)
    m_dataSetZeroMean.setCol(j, m_dataSet.getCol(j) - m_meanFace);

  // Here we get the MxM covariance matrix to calculate its eigenvectors
  // (where M is the number of training images). In this way we avoid
  // to calculate the N eigenvectors of the original covariance matrix NxN
  // (where N is the number of pixels in images)

  Matrix covarianceMatrix;	// (At*A)/M
  m_dataSetZeroMean.getTranspose(covarianceMatrix);
  covarianceMatrix *= m_dataSetZeroMean;
  covarianceMatrix /= m_dataSetZeroMean.cols();

  //std::cout << "covarianceMatrix = " << covarianceMatrix.rows() << " x " << covarianceMatrix.cols() << "\n";

  // We calculate eigenvectors (this can take a while)...
  try {
    covarianceMatrix.eig_sym(m_eigenvalues,
			     m_eigenvectors);
  }
  catch (std::exception& e) {
    return false;
  }

  // We sort the eigenvectors in descending order by its
  // corresponding eigenvalue significance
#if 1
  for (int i=0; i<m_eigenvalues.size(); ++i) { // TODO replace bubble-sort with qsort
    for (int j=i+1; j<m_eigenvalues.size(); ++j) {
      if (std::fabs(m_eigenvalues(j)) > std::fabs(m_eigenvalues(i))) {
	// Swap eigenvalues
	std::swap(m_eigenvalues(i), m_eigenvalues(j));
	// Swap eigenvectors
	{
	  Vector aux = m_eigenvectors.getCol(j);
	  m_eigenvectors.setCol(j, m_eigenvectors.getCol(i));
	  m_eigenvectors.setCol(i, aux);
	}
      }
    }
  }
#endif

#if 1
  std::cout << "----------------------------------------------------------------------\n";
  std::cout << "Eigenvalues (" << m_eigenvalues.size() << "):\n";
  double accum = 0.0, total = 0.0;
  for (int i=0; i<m_eigenvalues.size(); ++i)
    total += m_eigenvalues(i);

  std::cout << 0 << "\t" << 0 << "\n";

  for (size_t i=0; i<m_eigenvalues.size(); ++i) {
    accum += m_eigenvalues(i);
    std::cout << (i+1) << "\t" << (accum/total) << "\t" << "\n";
  }
  std::cout << "----------------------------------------------------------------------\n";
#endif

  return true;
}

/// Calculates the eigenfaces.
///
/// @warning You have to call #calculateEigenvalues before.
///
void Eigenfaces::calculateEigenfaces(size_t components)
{
  if (components < 1 && components > m_eigenvalues.size()) {
    char buf[1024];
    std::sprintf(buf, "Invalid argument components=%d in Eigenfaces::calculateEigenfaces method.\n"
		      "It is not between 1 and %d.",
		 components, m_eigenvalues.size());
    throw std::invalid_argument(std::string(buf));
  }

  m_eigenfaceComponents = components;

  // Calculate eigenfaces...
  m_eigenfaces.resize(m_pixelsPerImage,	// Rows
		      m_eigenfaceComponents); // Columns

  Vector eigenface(m_pixelsPerImage);
  for (size_t i=0; i<m_eigenfaceComponents; ++i) {
    eigenface.zero();
    for (size_t j=0; j<m_dataSetZeroMean.cols(); ++j)
      eigenface += m_eigenvectors(i, j) * m_dataSetZeroMean.getCol(j);

    m_eigenfaces.setCol(i, eigenface);
  }
}

/// Returns the number of eigenfaces required to represent the
/// specified level of variance.
///
/// @param variance
///   1.0 means all the variance, 0.8 is 80%, etc.
/// @return
///   The number of components for eigenfaces. Then you should call
///   #calculateEigenfaces with this returned value.
///
size_t Eigenfaces::getNumComponentsFor(double variance) const
{
  double total = 0.0;
  for (size_t i=0; i<m_eigenvalues.size(); ++i)
    total += m_eigenvalues(i);

  double accum = 0.0;
  for (size_t i=0; i<m_eigenvalues.size(); ++i) {
    accum += m_eigenvalues(i);
    if (accum/total >= variance)
      return i+1;
  }

  return m_eigenvalues.size();
}

/// Projects the image specified @a faceImage into the eigenspace,
/// returning its "facespacePoint".
///
/// @param faceImage
///   Face to be projected in the eigenspace.
///
/// @param eigenspacePoint
///   Resulting point in the eigenspace.
///
void Eigenfaces::projectInEigenspace(const Vector& faceImage, Vector& eigenspacePoint) const
{
  eigenspacePoint.resize(m_eigenfaceComponents);
  for (size_t k=0; k<m_eigenfaceComponents; ++k)
    eigenspacePoint(k) = m_eigenfaces.getCol(k) * (faceImage - m_meanFace);
}

//////////////////////////////////////////////////////////////////////
// Binary I/O
//////////////////////////////////////////////////////////////////////

void Eigenfaces::save(const char* filename) const
{
  std::ofstream f(filename, std::ios::binary);
  write(f);
}

void Eigenfaces::load(const char* filename)
{
  std::ifstream f(filename, std::ios::binary);
  read(f);
}

void Eigenfaces::write(std::ostream& s) const
{
  size_t eigenvalues_size = m_eigenvalues.size();
  size_t meanFace_size = m_meanFace.size();
  size_t eigenvectors_rows = m_eigenvectors.rows();
  size_t eigenvectors_cols = m_eigenvectors.cols();
  size_t eigenfaces_rows = m_eigenfaces.rows();
  size_t eigenfaces_cols = m_eigenfaces.cols();

  s.write((char*)&eigenvalues_size, sizeof(size_t));
  s.write((char*)m_eigenvalues.getRaw(), sizeof(double)*eigenvalues_size);

  s.write((char*)&meanFace_size, sizeof(size_t));
  s.write((char*)m_meanFace.getRaw(), sizeof(double)*meanFace_size);

  s.write((char*)&eigenvectors_rows, sizeof(size_t));
  s.write((char*)&eigenvectors_cols, sizeof(size_t));
  s.write((char*)m_eigenvectors.getRaw(), sizeof(double)*eigenvectors_rows*eigenvectors_cols);

  s.write((char*)&eigenfaces_rows, sizeof(size_t));
  s.write((char*)&eigenfaces_cols, sizeof(size_t));
  s.write((char*)m_eigenfaces.getRaw(), sizeof(double)*eigenfaces_rows*eigenfaces_cols);
}

void Eigenfaces::read(std::istream& s)
{
  size_t eigenvalues_size;
  size_t meanFace_size;
  size_t eigenvectors_rows;
  size_t eigenvectors_cols;
  size_t eigenfaces_rows;
  size_t eigenfaces_cols;

  s.read((char*)&eigenvalues_size, sizeof(size_t));
  m_eigenvalues.resize(eigenvalues_size);
  s.read((char*)m_eigenvalues.getRaw(), sizeof(double)*eigenvalues_size);

  s.read((char*)&meanFace_size, sizeof(size_t));
  m_meanFace.resize(meanFace_size);
  s.read((char*)m_meanFace.getRaw(), sizeof(double)*meanFace_size);

  s.read((char*)&eigenvectors_rows, sizeof(size_t));
  s.read((char*)&eigenvectors_cols, sizeof(size_t));
  m_eigenvectors.resize(eigenvectors_rows, eigenvectors_cols);
  s.read((char*)m_eigenvectors.getRaw(), sizeof(double)*eigenvectors_rows*eigenvectors_cols);

  s.read((char*)&eigenfaces_rows, sizeof(size_t));
  s.read((char*)&eigenfaces_cols, sizeof(size_t));
  m_eigenfaces.resize(eigenfaces_rows, eigenfaces_cols);
  s.read((char*)m_eigenfaces.getRaw(), sizeof(double)*eigenfaces_rows*eigenfaces_cols);

  m_eigenfaceComponents = eigenfaces_cols;
}
